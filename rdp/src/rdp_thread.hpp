#ifndef __RDP_THREAD_HPP__
#define __RDP_THREAD_HPP__
#include "rdp_time.hpp"
#include "rdp_shared.hpp"
namespace rdp {
/**
 * 可运行接口
 * 有四个状态,
 * - kWaiting 等待放入线程运行
 * - kRunning 已经在线程中运行
 * - kTerminated 线程结束
 */
class Runnable
{
public:
    enum State
    {
        /** 等待线程运行 */
        kWaiting    = 0,
        /** 已经开始运行 */
        kRunning    = 1,
        /** 已经等待回收 */
        kJoining    = 2,
        /** 线程运行结束 */
        kTerminated = 3,
    };

    Runnable():state_(kTerminated){}

	virtual ~Runnable();
    /** 线程中运行函数 */
    virtual void run(void) = 0;

    /** 当前是否处于运行状态 */
    enum State state() const { return state_; }
private:
    friend class Thread;
    enum State state_;
};

#if __cplusplus < 201402L
///读写互斥量
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

/** 线程类.
 * 该对象销毁时,并不能强制停止线程,因此
 * 必须调用join()或detach()指定线程结束.
 * 注意runnable的生存周期.
 */
class Thread : public Runnable
{
public:
    Thread():handle_(0){}

    /** 开始线程运行,
     * 若runnable存在, 执行外部runnable, 否则执行自身
     */
    void start(const SharedPtr<Runnable>& runnable = SharedPtr<Runnable>());

    /**
     * 等待运行结束,回收线程资源
     */
    void join(void);

    /** 是否需要回收线程 */
    bool joinable(void) const { return handle_ != 0; }

    /**
     * 使外部的runnable,脱离控制,独立在线程运行
     */ 
    void detach(void);

    virtual void run(void){}
public:
    /** 以分离线程运行 */
    static void run(const SharedPtr<Runnable>& runnable);

    /**
     * 线程入口, 内部函数
     * 必须保证runnable在运行期间有效
     */
    static void _routine(Runnable* runnable);
private:
    void* handle_;
};
#if __cplusplus < 201103L
///可递归互斥量
struct Mutex final
{
    Mutex():impl_(0){}
    ~Mutex();
    void lock() const;
    void unlock() const;
private:
    mutable void* impl_;
};
///局部的锁
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
