#ifndef __RDP_THREAD_HPP__
#define __RDP_THREAD_HPP__
#include "rdp_time.hpp"
#include "rdp_shared.hpp"
namespace rdp {
/**
 * �����нӿ�
 * ���ĸ�״̬,
 * - kWaiting �ȴ������߳�����
 * - kRunning �Ѿ����߳�������
 * - kTerminated �߳̽���
 */
class Runnable
{
public:
    enum State
    {
        /** �ȴ��߳����� */
        kWaiting    = 0,
        /** �Ѿ���ʼ���� */
        kRunning    = 1,
        /** �Ѿ��ȴ����� */
        kJoining    = 2,
        /** �߳����н��� */
        kTerminated = 3,
    };

    Runnable():state_(kTerminated){}

	virtual ~Runnable();
    /** �߳������к��� */
    virtual void run(void) = 0;

    /** ��ǰ�Ƿ�������״̬ */
    enum State state() const { return state_; }
private:
    friend class Thread;
    enum State state_;
};
struct Lockable
{
    virtual void lock() const = 0;
    virtual void unlock() const = 0;
};
///�ɵݹ黥����
struct Mutex : public Lockable
{
    Mutex():impl_(0){}
    ~Mutex();
    virtual void lock() const;
    virtual void unlock() const;
private:
    friend struct RWMutex;
    mutable void* impl_;
};

///��д������
struct RWMutex
{
#pragma warning(disable:4355) 
    RWMutex():write_(*this),read_(*this), impl_(0){}
#pragma warning(default:4355) 
    ~RWMutex();
    const Lockable& writeMutex() const { return write_; }
    const Lockable& readMutex() const { return read_; }
    void writeLock() const;
    void writeUnlock() const;
    void readLock() const;
    void readUnlock() const;
private:
    struct WriteMutex: public Lockable
    {
        RWMutex& rw_;
        WriteMutex(RWMutex& rw):rw_(rw){}
        virtual void lock() const { rw_.writeLock(); }
        virtual void unlock() const { rw_.writeUnlock(); }
    };
    struct ReadMutex: public Lockable
    {
        RWMutex& rw_;
        ReadMutex(RWMutex& rw):rw_(rw){}
        virtual void lock() const { rw_.readLock(); }
        virtual void unlock() const { rw_.readUnlock(); }
    };
    const WriteMutex write_;
    const ReadMutex read_;
    mutable void* impl_;
};

///�ֲ�����
struct Guard
{
    const Lockable& lock_;
    Guard(const Lockable& lock):lock_(lock) { lock_.lock(); }
    ~Guard() { lock_.unlock(); }
};

/** �߳���.
 * �ö�������ʱ,������ǿ��ֹͣ�߳�,���
 * �������join()��detach()ָ���߳̽���.
 * ע��runnable����������.
 */
class Thread : public Runnable
{
public:
    Thread():handle_(0){}

    /** ��ʼ�߳�����,
     * ��runnable����, ִ���ⲿrunnable, ����ִ������
     */
    void start(const SharedPtr<Runnable>& runnable = SharedPtr<Runnable>());

    /**
     * �ȴ����н���,�����߳���Դ
     */
    void join(void);

    /** �Ƿ���Ҫ�����߳� */
    bool joinable(void) const { return handle_ != 0; }

    /**
     * ʹ�ⲿ��runnable,�������,�������߳�����
     */ 
    void detach(void);

    virtual void run(void){}
public:
    /** �Է����߳����� */
    static void run(const SharedPtr<Runnable>& runnable);

    /**
     * �߳����, �ڲ�����
     * ���뱣֤runnable�������ڼ���Ч
     */
    static void _routine(Runnable* runnable);
private:
    void* handle_;
};
}
#endif
