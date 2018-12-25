#include "rdp_config.hpp"
#include "rdp_transport.hpp"
#include "rdp_thread.hpp"
#include "rdp_ringbuffer.hpp"
#include "rdp_bitstream.hpp"
namespace rdp {
namespace transport {
namespace {
class Disturber : public DisturberIFace, public Thread 
{
    virtual void run();
    void enqueue(BitStream& bs, const Time& now);
    void perform(BitStream& bs, const Time& now)
    {
        if (receiver_)
            receiver_->onReceived_R(bs, now);
        if (sender_)
            sender_->send_W(bs, now);
    }
    virtual void onReceived_R(BitStream& bs, const Time& now)
    {
        enqueue(bs, now);
    }
    virtual void send_W(BitStream& bs, const Time& now)
    {
        enqueue(bs, now);
    }
    virtual void restart(const Status& status)
    {
        join();
        status_ = status;
        start();
    }
    struct Packet
    {
        uint8_t bitBuffer[kMaxMtuBytes];
        int     bitWrite;
        Time    sendTime;
        void assign(BitStream& bs, const Time& time)
        {
            this->sendTime = time;
            this->bitWrite = bs.bitWrite;
            memcpy(this->bitBuffer, bs.bitBuffer, bits2bytes(bs.bitWrite));
        }
    };
    bool active_;
    ReceiverIFace*  receiver_;
    SenderIFace*    sender_;

    Status status_;
    Mutex  packetsMutex_;
    float accumulatedJitterMS_;
    RingBuffer<Packet> packets_;
    ~Disturber()
    {
        active_ = false;
        join();
    }
public:
    Disturber(SenderIFace* sender, ReceiverIFace* receiver, const Status& status)
        : sender_(sender),receiver_(receiver), accumulatedJitterMS_(0),
          active_(true),packets_(100), status_(status)
    {
        start();
    }
};

void Disturber::enqueue(BitStream& bs, const Time& now)
{
    if (state() != kRunning) {
        perform(bs, now);
        return;
    }
    Time jitter;
    if (status_.jitter > Time::MS(0)){//随机的瞬时抖动
        jitter = Time::MS(rand() % status_.jitter.millisec());
        if (accumulatedJitterMS_ > 0){
            if (status_.delay - jitter > Time::MS(0))
                jitter = -jitter;
            else
                jitter = -status_.delay;
        }
    }
    Time sendTime(now + status_.delay + jitter);//绝对发送时刻
    {
        Guard _(packetsMutex_);
        if(status_.queueLength > 0) {//保持队列长度
            while (packets_.size() >= status_.queueLength)
                packets_.pop_front();
        }
        if (packets_.size() == 0) {
            packets_.push_back().assign(bs, sendTime);
        }
        else if (status_.disordPercent > 0 
                 && packets_.size() > 1
                 &&  (rand() % 100 <=  status_.disordPercent)) {//随机的瞬时乱序
            //查找发送前可参与乱序的个数
            int count=0;
            for(int i=packets_.size()-1;i>=0; ++i) {
                if (packets_[i].sendTime < sendTime)
                    break;
                ++count;
            }
            const int pos = rand() % count;
            //插入并增加后续包的延时,以保证递增关系,方便处理
            packets_.push_back();
            Packet& item = packets_[pos];
            const Time delta(sendTime - item.sendTime);
            for(int i=packets_.size()-1; i>pos; --i) {
                Packet& p = packets_[i];
                p = packets_[i-1];
                p.sendTime += delta;
            }
            sendTime = item.sendTime + delta/2;
            jitter = sendTime - (now + status_.delay);
            item.assign(bs, sendTime);
        }
        else {
            //抖动到前个位置,复位以保证递增关系,方便处理
            Packet& back = packets_.back();
            if (back.sendTime > sendTime){
                sendTime = back.sendTime;
                jitter = sendTime - (now + status_.delay);
            }
            packets_.push_back().assign(bs, sendTime);
        }
    }
    if(status_.jitter > Time::MS(0))
        accumulatedJitterMS_ = accumulatedJitterMS_*15.0f/16.0f + jitter.millisec()/16.0f;
}
void Disturber::run()
{
    Time wait;
    uint8_t bitBuffer[kMaxMtuBytes];
    BitStream bs(bitBuffer, kMaxMtuBytes);
    while (active_) {
        packetsMutex_.lock();
        if (packets_.size() <= 0){
            packetsMutex_.unlock();
            wait = Time::MS(15);
        }
        else while (packets_.size() > 0) {
            const Time now(Time::now());
            Packet& front = packets_.front();
            if (front.sendTime > now) {
                wait = front.sendTime - now;
                packetsMutex_.unlock();
                break;
            }
            packets_.pop_front();
            if (status_.lostPercent  == 0 || (rand() % 100 > status_.lostPercent )) {//不丢包
                memcpy(bs.bitBuffer, front.bitBuffer, kMaxMtuBytes);
                bs.bitRead = 0;
                bs.bitWrite = front.bitWrite;
                packetsMutex_.unlock();
                perform(bs, now);
                packetsMutex_.lock();;
            }
        }
        Time::sleep(wait);
    }
}
}//namespace

ReceiverIFace* DisturberIFace::create(ReceiverIFace& receiver, const Status& status)
{
    return new Disturber(nullptr, &receiver, status);
}
SenderIFace* DisturberIFace::create(SenderIFace& sender, const Status& status)
{
    return new Disturber(&sender, nullptr, status);
}

}//namespace transport
}//namespace rdp
