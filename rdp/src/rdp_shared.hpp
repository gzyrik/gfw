#ifndef __RDP_SHARED_HPP__
#define __RDP_SHARED_HPP__
#include "rdp_basictypes.hpp"
#include <algorithm>
namespace rdp {
/** 共享计数的基类 */
class SharedCountBase
{
    friend class SharedCount;
    friend class WeakCount;
    int refcount_;
    int weakcount_;
protected:
    SharedCountBase():refcount_(1),weakcount_(1){}
    virtual ~SharedCountBase(){}
    virtual void dispose()=0;
    virtual void destroy() { delete this; }
    bool addLockRef();
    void addRef();
    void release();
    void weakAddRef();
    void weakRelease();
    int use_count() const { return refcount_; }
};
class WeakCount;
struct StaticCastTag {};
template<typename X> struct NullDeleter
{
    void operator()(X*) const {}
};

/** 共享计数的接口包装类.
 * 内部委托SharedCountImpl子类实现
 */
class SharedCount
{
    friend class WeakCount;
    template<typename Y> friend class SharedPtr;

    /** 共享计数标准删除的实现类 */
    template<typename X> struct SharedCountImpl : public SharedCountBase
    {
        X* ptr_;
        explicit SharedCountImpl(X*p):ptr_(p){}
        virtual void dispose() { delete ptr_; }
    };
    /** 共享计数托管删除的实现类 */
    template<typename X, typename D> struct SharedCountImplD : public SharedCountBase
    {
        X* ptr_;
        D  deleter_;
    public:
        explicit SharedCountImplD(X*p, const D& d):ptr_(p),deleter_(d) {}
        virtual void dispose() { deleter_(ptr_); }
    };

    SharedCountBase *impl_;

    SharedCount():impl_(0) {}
    template<typename Y> explicit SharedCount(Y*p):impl_(0)
    {
        impl_ = new SharedCountImpl<Y>(p);
    }
    template<typename Y,typename D> SharedCount(Y*p, const D& d) :impl_(0)
    {
        impl_=new SharedCountImplD<Y,D>(p,d);
    }
    ~SharedCount() { if(impl_)impl_->release(); }
    SharedCount(SharedCount const& r) :impl_(r.impl_) { if(impl_) impl_->addRef(); }

    explicit SharedCount(WeakCount const& r);

    SharedCount& operator=(SharedCount const& r)
    {
        SharedCountBase* tmp=r.impl_;
        if(tmp!=impl_)
        {
            if(tmp) tmp->addRef();
            if(impl_) impl_->release();
            impl_ = tmp;
        }
        return *this;
    }
    void swap(SharedCount& r) { std::swap(impl_, r.impl_); }
    int use_count() const { return impl_ ? impl_->use_count() : 0; }
    bool unique() const { return impl_->use_count()==1; }
    bool empty() const { return impl_==0; }
    friend inline bool operator==(SharedCount const& a, SharedCount const& b) { return a.impl_==b.impl_; }
    friend inline bool operator< (SharedCount const& a, SharedCount const& b) { return a.impl_<b.impl_; }
};
class WeakCount
{
    friend class SharedCount;
    template<typename Y> friend class WeakPtr;
    SharedCountBase *impl_;

    WeakCount():impl_(0){}
    WeakCount(SharedCount const& r):impl_(r.impl_)
    {
        if(impl_)impl_->weakAddRef();
    }

    WeakCount(WeakCount const & r):impl_(r.impl_)
    {
        if(impl_)impl_->weakAddRef();
    }
    ~WeakCount()
    {
        if(impl_)impl_->weakRelease();
    }
    WeakCount& operator=(SharedCount const& r)
    {
        SharedCountBase* tmp=r.impl_;
        if(tmp!=impl_)
        {
            if(tmp) tmp->weakAddRef();
            if(impl_)impl_->weakRelease();
            impl_=tmp;
        }
        return *this;
    }
    WeakCount& operator=(WeakCount const& r)
    {
        SharedCountBase* tmp=r.impl_;
        if(tmp!=impl_) {
            if(tmp) tmp->weakAddRef();
            if(impl_)impl_->weakRelease();
            impl_=tmp;
        }
        return *this;
    }
    void swap(WeakCount& r) { std::swap(impl_, r.impl_); }
    int use_count()const { return impl_ ? impl_->use_count() : 0; }
    friend inline bool operator==(WeakCount const& a, WeakCount const& b) { return a.impl_ == b.impl_; }
    friend inline bool operator< (WeakCount const& a, WeakCount const& b) { return a.impl_ < b.impl_; }
};
inline SharedCount::SharedCount(WeakCount const& r):impl_(r.impl_)
{
    if(impl_ && !impl_->addLockRef())
        impl_=0;
}
template<typename T>
class SharedPtr
{
private:
    template<typename Y> friend class SharedPtr;
    template<typename Y> friend class WeakPtr;
    T* ptr_;
    SharedCount count_;
public:
    SharedPtr():ptr_(0), count_()
    {}
    template<typename Y> explicit SharedPtr(Y*p)
        :ptr_(p),count_(p)
    {}
    template<typename Y, typename D>SharedPtr(Y*p, const D& d)
        :ptr_(p),count_(p,d)
    {}
    SharedPtr& operator=(SharedPtr const& r)
    {
        ptr_=r.ptr_;
        count_=r.count_;
        return *this;
    }
    template<typename Y> explicit SharedPtr(WeakPtr<Y> const& r)
        :ptr_(0),count_(r.count_)
    {
        if(!count_.empty()) ptr_=r.ptr_;
    }
    template<typename Y> SharedPtr(SharedPtr<Y> const& r)
        :ptr_(r.ptr_),count_(r.count_)
    {
    }
    //与类型为Y的r共享计数,但类型却为T
    template<typename Y> SharedPtr(SharedPtr<Y> const& r, T*p)
        :ptr_(p), count_(r.count_)
    {}
    //允许向下转化
    template<class Y> SharedPtr(SharedPtr<Y> const& r, StaticCastTag)
        :ptr_(static_cast<T*>(r.ptr_)), count_(r.count_)
    {}
    template<typename Y> SharedPtr& operator=(SharedPtr<Y>const& r)
    {
        ptr_=r.ptr_;
        count_=r.count_;
        return *this;
    }
    void reset()
    {
        SharedPtr<T>().swap(*this);
    }
    template<typename Y> void reset(Y*p)
    {
        SharedPtr<T>(p).swap(*this);
    }
    template<typename Y, typename D>void reset(Y*p,D d)
    {
        SharedPtr<T>(p,d).swap(*this);
    }
    template<typename Y> void reset(SharedPtr<Y> const& r, T*p)
    {
        SharedPtr<T>(r,p).swap(*this);
    }
    T& operator* () const
    {
        return *ptr_;
    }
    T* operator->() const
    {
        return ptr_;
    }
    T* get() const
    {
        return ptr_;
    }
    operator bool()const
    {
        return ptr_!=0;
    }
    bool operator! () const
    {
        return ptr_==0;
    }
    bool unique() const
    {
        return count_.unique();
    }
    int use_count() const
    {
        return count_.use_count();
    }
    void swap(SharedPtr<T>& other)
    {
        std::swap(ptr_, other.ptr_);
        count_.swap(other.count_);
    }
    template<class U> static SharedPtr cast(SharedPtr<U> const & r)
    {
        return SharedPtr<T>(r, StaticCastTag());
    }
    template<class T> static SharedPtr<T> ever(T* p)
    {
        return SharedPtr<T>(p, NullDeleter<T>());
    }
};

template<typename T>
class WeakPtr
{
private:
    template<typename Y> friend class SharedPtr;
    template<typename Y> friend class WeakPtr;
    T* ptr_;
    WeakCount count_;
public:
    WeakPtr():ptr_(0),count_()
    {}
    template<typename Y> WeakPtr(WeakPtr<Y> const& r)
        :count_(r.count_)
    {
        ptr_=r.lock().get();
    }
    template<typename Y> WeakPtr(SharedPtr<Y>const&r)
        :ptr_(r.ptr_),count_(r.count_)
    {}
    template<typename Y> WeakPtr& operator=(WeakPtr<Y>const& r)
    {
        ptr_ = r.lock().get();
        count_ = r.count_;
        return *this;
    }
    template<typename Y> WeakPtr& operator=(SharedPtr<Y>const&r)
    {
        ptr_=r.ptr_;
        count_=r.count_;
        return *this;
    }
    SharedPtr<T> lock() const
    {
        return SharedPtr<T>(*this);
    }
    int use_count() const
    {
        return count_.use_count();
    }
    bool expired() const
    {
        return count_.use_count()==0;
    }
    void reset()
    {
        WeakPtr<T>().swap(*this);
    }
    void swap(WeakPtr<T>&other)
    {
        std::swap(ptr_, other.ptr_);
        count_.swap(other.count_);
    }
};
}
#endif
