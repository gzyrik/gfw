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

#if __cplusplus < 201402L
///��д������
struct RWMutex final
{
    RWMutex(): impl_(0){}
    ~RWMutex();
    void lock() const;
    void unlock() const;
    void lock_shared() const;
    void unlock_shared() const;
private:
    mutable void* impl_;
};
#endif

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
#if __cplusplus < 201103L
///�ɵݹ黥����
struct Mutex final
{
    Mutex():impl_(0){}
    ~Mutex();
    void lock() const;
    void unlock() const;
private:
    mutable void* impl_;
};
///�ֲ�����
struct Guard final
{
    const Mutex& lock_;
    Guard(const Mutex& lock):lock_(lock) { lock_.lock(); }
    ~Guard() { lock_.unlock(); }
};
template<typename T>
struct Atomic
{
    T operator++() {
#ifdef _WIN32
        return (T)InterlockedIncrement(&val);
#endif
    }
    T operator++ (int) {
#ifdef _WIN32
        return (T)InterlockedIncrement(&val)-1;
#endif
    }
    operator T() const {
        return (T)val;
    }
private:
#ifdef _WIN32
    long volatile val;
#endif
};
#else //C++11
template<typename T>
using Atomic = std::atomic<T>; 
template<typename T>
using Guard = std::lock_guard<T>;
typedef std::recursive_mutex Mutex;
typedef std::shared_mutex RWMutex;//c++17
#endif
}
#endif
