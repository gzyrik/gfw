#include "rdp_config.hpp"
#include "rdp_time.hpp"

#ifdef HAVE_TIMEGETTIME
#include <windows.h>
#pragma comment( lib,"winmm.lib" )
inline static uint64_t timeMS()
{
    static volatile LONG lastTimeGetTime = 0;
    static volatile uint64_t numWrapTimeGetTime = 0;
    volatile LONG* lastTimeGetTimePtr = &lastTimeGetTime;
    DWORD now = timeGetTime();
    // Atomically update the last gotten time
    DWORD old = InterlockedExchange(lastTimeGetTimePtr, now);
    if(now < old)
    {
        // If now is earlier than old, there may have been a race between
        // threads.
        // 0x0fffffff ~3.1 days, the code will not take that long to execute
        // so it must have been a wrap around.
        if(old > 0xf0000000 && now < 0x0fffffff) 
        {
            numWrapTimeGetTime++;
        }
    }
    return (uint64_t)now + (numWrapTimeGetTime<<32);
}
#elif defined HAVE_MACH_TIMEBASE_INFO
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <time.h>
namespace {
static mach_timebase_info_data_t _timebase;
struct InitMachTimeBaseInfo
{
    InitMachTimeBaseInfo()
    {
        mach_timebase_info(&_timebase);
    }
};
static struct InitMachTimeBaseInfo _;
}
inline static uint64_t machNS()
{
    return (mach_absolute_time()*_timebase.numer/_timebase.denom);
}
#endif
namespace rdp {
/** 返回当前时间 */
Time Time::now(void)
{
#ifdef HAVE_TIMEGETTIME
    return Time::MS(::timeMS());
#elif defined HAVE_MACH_TIMEBASE_INFO
    return Time::NS(::machNS());
#elif defined(HAVE_CLOCK_GETTIME)
    /* librt clock_gettime() is our first choice */
    struct timespec ts;
#ifdef CLOCK_MONOTONIC
    clock_gettime (CLOCK_MONOTONIC, &ts);
#else
    clock_gettime (CLOCK_REALTIME, &ts);
#endif
    return  Time::S(ts.tv_sec) + Time::NS(ts.tv_nsec);
#else
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return  Time::S(tv.tv_sec) + Time::US(tv.tv_usec);
#endif
}

void Time::sleep(const Time& delta)
{
#ifdef _WIN32
    ::Sleep((DWORD)delta.millisec());
#else
    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = delta.nanosec();
    nanosleep(&tim, NULL);
#endif
}
}
