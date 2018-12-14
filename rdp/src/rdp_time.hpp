#ifndef __RDP_TIME_HPP__
#define __RDP_TIME_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/** 
 * 抽象时间对象.
 * 设 Timer t, 若计算相应秒,毫秒,两种方法
 * - t.sec(), t.millisec()
 * - t/Timer::kSecond, t/Timer::kMillisecond
 * 第二种方法,连乘时可以减少累积误差
 * Windows上默认精度为15 MS,
 * 使用timeBeginPeriod(1)/timeEndPeriod(1),可以调整到1 MS
 * 但会占用 CPU
 * 其他默认精度2MS
 */
struct Time
{
private:
	enum TimeUnit
	{
		kMicrosecond = 1,
		kMillisecond = 1000,
		kSecond      = 1000000,
	};
	Time(int64_t value):value_(value){}
	/** 以指定的时间和单位构造,对应的时间对象 */
	Time(int64_t value, enum TimeUnit unit):value_(value*unit)
	{}
	int64_t value_;//microsecond
public:
    /** 默认构造,时间为0 */
    Time():value_(0)
    {}
    /** 复制构造 */
    Time(const Time& t):value_(t.value_)
    {}
public:
    /** 休眠 */
    static void sleep(const Time& time);
    /** 返回当前时间 */
    static Time now(void);
    /** 以纳秒构造时间对象 */
    static Time NS(uint64_t nanosec)
    {
        return US(nanosec/1000);
    }
    /** 以微秒构造时间对象 */
    static Time US(int64_t micorsec)
    {
        return Time(micorsec, kMicrosecond);
    }
    /** 以毫秒构造时间对象 */
    static Time MS(int64_t millisec)
    {
        return Time(millisec, kMillisecond);
    }
    /** 以秒构造时间对象 */
    static Time S(int64_t sec)
    {
        return Time(sec, kSecond);
    }
public:
    /** 返回纳秒单位的值*/
    int64_t nanosec() const
    {
        return microsec()*1000;
    }
    /** 返回微秒单位的值*/
    int64_t microsec() const
    {
        return value_/kMicrosecond;
    }
    /** 返回毫秒单位的值*/
    int64_t millisec() const
    {
        return value_/kMillisecond;
    }
    /** 返回秒单位的值*/
    int64_t sec() const
    {
        return value_/kSecond;
    }
	/** 返回次数,hz为每秒次数 */
	int64_t count(int64_t hz) const
	{
		return value_*hz/kSecond;
	}
	int64_t hertz(int64_t count) const
	{
		return count*kSecond/value_;
	}
public:
    Time& operator-= (const Time& t1)
    {
        value_ -= t1.value_;
        return *this;
    }
    Time& operator+= (const Time& t1)
    {
        value_ += t1.value_;
        return *this;
    }
    operator int64_t() const
    {
        return value_;
    }
    /*
       friend bool operator> (const Time& t1, const Time& t2)
       {
       return t1.value_ > t2. value_;
       }
       friend bool operator< (const Time& t1, const Time& t2)
       {
       return t1.value_ < t2. value_;
       }
       friend bool operator!= (const Time& t1, const Time& t2)
       {
       return t1.value_ != t2. value_;
       }
       operator bool() const
       {
       return value_ != 0;
       }
       */
    friend Time operator+ (const Time& t1, const Time& t2)
    {
        return Time(t1.value_ + t2. value_);
    }
    friend Time operator- (const Time& t1, const Time& t2)
    {
        return Time(t1.value_ - t2. value_);
    }
    friend Time operator/ (const Time& t1, const int t2)
    {
        return Time(t1.value_/t2);
    }
    friend Time operator- (const Time& t1)
    {
        return Time(-t1.value_);
    }
    Time operator >> (int val)
    {
        return Time(value_ >> val);
    }
};

}
#endif
