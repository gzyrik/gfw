#ifndef __RDP_RINGBUFFER_HPP__
#define __RDP_RINGBUFFER_HPP__
#include "rdp_time.hpp"
#include <algorithm>
namespace rdp {
/**
 * �����͵Ļ��ζ���
 * @param[in] T Ԫ������
 */
template<typename T> class RingBuffer
{
private:
    int locate(const int i) const
    {
        return (i + capacity_) & (capacity_ - 1);
    }
    //����,����һ��,��Ӧ��
    //{--><--} => {-->....<--}
    //{<---->} => {<---->....}
    void expand()
    {
        T* const array = array_;
        const int capacity = capacity_;

        capacity_ <<= 1;
        array_ = new T[capacity_];
        std::copy(array, array + tail_, array_);
        if (tail_ < head_) {
            std::copy(array+head_, array+capacity, array_ + capacity + head_);
            head_ += capacity;
        }
        delete array;
    }
    int head_, tail_, capacity_;
    T *array_;
public:
    //��ʼ��������
    RingBuffer(const int capacity):array_(0)
    {
        reset(capacity);
    }
    ~RingBuffer()
    {
        delete array_;
    }
    int size() const
    {
        return locate(tail_ - head_);
    }
    int capacity() const
    {
        return capacity_;
    }
    void reset(int capacity)
    {
        int i;
        for(i=0; capacity > (1<<i); ++i);
        delete array_;
        tail_ = head_ = 0;
        capacity_ = 1 << i;
        array_ = new T[capacity_];
    }
    T& back(void)
    {
        return array_[locate(tail_ - 1)];
    }
    const T& back(void) const
    {
        return array_[locate(tail_ - 1)];
    }
    T& push_back()
    {
        T* p;
        const int back = locate(tail_ + 1);
        if (back == head_){
            expand();
            p = array_+tail_;
            tail_ = locate(tail_ + 1);
        }
        else {
            p = array_+tail_;
            tail_ = back;
        }
        return *p;
    }
    void push_back(const T& t)
    {
        push_back() = t;
    }
    void pop_back(void)
    {
        if (tail_ != head_)
            tail_ = locate(tail_ - 1);
    }
    T& front(void)
    {
        return array_[head_];
    }
    const T& front(void) const
    {
        return array_[head_];
    }
    T& push_front()
    {
        const int front = locate(head_ - 1);
        if (front == tail_) {
            expand();
            head_ = locate(head_ - 1);
        }
        else {
            head_ = front;
        }
        return array_[head_];
    }
    void push_front(const T& t)
    {
        push_front() = t;
    }
    void pop_front(void)
    {
        if (head_ != tail_)
            head_ = locate(head_ + 1);
    }
    T& operator[] (int index)
    {
        return array_[locate(head_+index)];
    }
};
/**
 * ���������Ĳ�����
 * @param[in] T ��������
 */
template <typename T>
class CapacitySampler
{
private:
    void discardExpired(const Time& now)
    {
        while (samples_.size() > 0) {
            Sample& front = samples_.front();
            if (front.stamp + duration_ >= now)
                break;
            accumulate_ -= front.value;
            samples_.pop_front();
        }
    }
    struct Sample 
    {
        Sample(){}
        Sample(const T& val, const Time& now):
            value(val),stamp(now)
        {}
        T value;
        Time stamp;
    };
    RingBuffer<Sample> samples_;
    T accumulate_;
    Time duration_;

public:
    /**
     * @param[in] duration ������Ч��
     * @param[in] hzMax �����ܵĲ���Ƶ��(ÿ�����)
     */
    CapacitySampler(const Time& duration, const int hzMax):
        samples_(static_cast<int>(duration.count(hzMax))),
        accumulate_(0),
        duration_(duration)
    {}
    //����
    void reset(const Time& duration, const int hzMax)
    {
        duration_ = duration;
        samples_.reset(static_cast<int>(duration.count(hzMax)));
    }
    /**
     * ���²���ֵ
     * @param[in] ����ֵ
     * @param[in] ��ǰʱ��
     */
    void update(const T& val, const Time& now)
    {
        discardExpired(now);
        if (samples_.size() == 0){
            samples_.push_back(Sample(val, now));
        }
        else {
            Sample& back = samples_.back();
            if (now <= back.stamp)
                back.value += val;
            else
                samples_.push_back(Sample(val, now));
        }
        accumulate_ += val;
    }
    /**
     * @param[in] ��ǰʱ��
     * @return ����ĵ�ǰ����
     */
    const T value(const Time& now)
    {
        discardExpired(now);
        if (samples_.size() <= 1)
            return accumulate_;

        const Time duration(now - samples_.front().stamp);
        return T(duration.hertz(accumulate_));
    }
};
}
#endif
