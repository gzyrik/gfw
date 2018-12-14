#ifndef __RDP_TIME_HPP__
#define __RDP_TIME_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/** 
 * ����ʱ�����.
 * �� Timer t, ��������Ӧ��,����,���ַ���
 * - t.sec(), t.millisec()
 * - t/Timer::kSecond, t/Timer::kMillisecond
 * �ڶ��ַ���,����ʱ���Լ����ۻ����
 * Windows��Ĭ�Ͼ���Ϊ15 MS,
 * ʹ��timeBeginPeriod(1)/timeEndPeriod(1),���Ե�����1 MS
 * ����ռ�� CPU
 * ����Ĭ�Ͼ���2MS
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
	/** ��ָ����ʱ��͵�λ����,��Ӧ��ʱ����� */
	Time(int64_t value, enum TimeUnit unit):value_(value*unit)
	{}
	int64_t value_;//microsecond
public:
    /** Ĭ�Ϲ���,ʱ��Ϊ0 */
    Time():value_(0)
    {}
    /** ���ƹ��� */
    Time(const Time& t):value_(t.value_)
    {}
public:
    /** ���� */
    static void sleep(const Time& time);
    /** ���ص�ǰʱ�� */
    static Time now(void);
    /** �����빹��ʱ����� */
    static Time NS(uint64_t nanosec)
    {
        return US(nanosec/1000);
    }
    /** ��΢�빹��ʱ����� */
    static Time US(int64_t micorsec)
    {
        return Time(micorsec, kMicrosecond);
    }
    /** �Ժ��빹��ʱ����� */
    static Time MS(int64_t millisec)
    {
        return Time(millisec, kMillisecond);
    }
    /** ���빹��ʱ����� */
    static Time S(int64_t sec)
    {
        return Time(sec, kSecond);
    }
public:
    /** �������뵥λ��ֵ*/
    int64_t nanosec() const
    {
        return microsec()*1000;
    }
    /** ����΢�뵥λ��ֵ*/
    int64_t microsec() const
    {
        return value_/kMicrosecond;
    }
    /** ���غ��뵥λ��ֵ*/
    int64_t millisec() const
    {
        return value_/kMillisecond;
    }
    /** �����뵥λ��ֵ*/
    int64_t sec() const
    {
        return value_/kSecond;
    }
	/** ���ش���,hzΪÿ����� */
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
