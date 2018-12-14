#ifndef __RDP_RWQUEUE_HPP__
#define __RDP_RWQUEUE_HPP__
namespace rdp {
template<typename T> class RWQueue
{
public:
    typedef T value_type;

    RWQueue (int initSize = 0);

    ~RWQueue();

    /** 写方式锁住队列,返回可写的位置,
     * 后续必须调用 writeUnlock 或 cancelWrite
     */
    T* writeLock(void);

    /** 撤消写入并解锁
     * @param[in] cancelToLocation 撤消的位置
     */
    void cancelWrite(T* cancelToLocation);

    /** 完成写入操作并解锁 */
    void writeUnlock(void);

    /** 读方式锁住队列,若空,返回0
     * @note 若返回非0, 则后续必须调用 readUnlock 或 cancelRead
     */
    T* readLock(void);

    /** 撤消读取并解锁
     * @param[in] cancelToLocation 撤消的位置
     */
    void cancelRead(T* cancelToLocation);

    /** 完成读取操作并解锁 */
    void readUnlock(void);

    /** 回收空闲的结点 */
    void shrink(void);

    /** 返回队列长度 */
    int size(void) const;

    /** 入队操作 */
    void push(const T& t)
    {
        *writeLock() = t;
        writeUnlock();
    }

private:
    struct Node 
    {
        T value;
        volatile struct Node *next;
        volatile bool readyToRead;
    };
    volatile Node *readAheadPtr_;
    volatile Node *writeAheadPtr_;
    volatile Node *readPtr_;
    volatile Node *writePtr_;
    int readCount_, writeCount_;
};
//-----------------------------------------------------------------------------
template<typename T> RWQueue<T>::RWQueue(int initSize)
{
    volatile Node *dataPtr = new Node;
    dataPtr -> readyToRead = false;
    writePtr_ = readPtr_= dataPtr;
    for(int i=1; i < initSize; ++i)
    {
        dataPtr = dataPtr -> next = new Node;
        dataPtr -> readyToRead = false;
    }
    dataPtr -> next = writePtr_;// 构成环形单向链表
    readAheadPtr_  = readPtr_;
    writeAheadPtr_ = writePtr_;
    readCount_ = writeCount_ = 0;
}
//-----------------------------------------------------------------------------
template<typename T> RWQueue<T>::~RWQueue()
{
    volatile Node *node = writePtr_;
    do {
        volatile Node* next=node->next;
        delete node;
        node=next;
    } while (node != writePtr_);
}
//-----------------------------------------------------------------------------
template<typename T> T* RWQueue<T>::writeLock()
{
    if(writeAheadPtr_->next == readPtr_
       || writeAheadPtr_->next->readyToRead)
    {
        Node* dataPtr = new Node;
        dataPtr -> readyToRead = false;
        dataPtr -> next        = writeAheadPtr_->next;
        writeAheadPtr_->next   = dataPtr;
    }
    volatile Node* last = writeAheadPtr_;
    writeAheadPtr_ = writeAheadPtr_->next;
    return (T*)(last);
}
//-----------------------------------------------------------------------------
template<typename T> void RWQueue<T>::cancelWrite(T* cancelToLocation)
{
    writeAheadPtr_ = (Node*)(cancelToLocation);
}
//-----------------------------------------------------------------------------
template<typename T> void RWQueue<T>::writeUnlock(void)
{
    writeCount_++;
    writePtr_->readyToRead = true;
    writePtr_ = writePtr_->next;
}
//-----------------------------------------------------------------------------
template<typename T> T* RWQueue<T>::readLock(void)
{
    if(readAheadPtr_==writePtr_||readAheadPtr_->readyToRead==false)
        return 0;

    volatile Node* last;
    last=readAheadPtr_;
    readAheadPtr_=readAheadPtr_->next;
    return (T*)(last);
}
//-----------------------------------------------------------------------------
template<typename T> void RWQueue<T>::cancelRead(T* cancelToLocation)
{
    readAheadPtr_=(Node*)(cancelToLocation);
}
//-----------------------------------------------------------------------------
template<typename T> void RWQueue<T>::readUnlock()
{
    readCount_++;
    readPtr_->readyToRead=false;
    readPtr_=readPtr_->next;
}
//-----------------------------------------------------------------------------
template<typename T> void RWQueue<T>::shrink(void)
{
    //[writeAheadPtr_, readPtr_)之间的结点是空闲的无效数据
    volatile Node *node = writeAheadPtr_->next;
    while(node != readPtr_)
    {
        assert(node->readyToRead==false);
        volatile Node* next=node->next;
        delete node;
        node=next;
    }
    //再次建立环形链接
    writeAheadPtr_->next = readPtr_;
}
//-----------------------------------------------------------------------------
template<typename T> int RWQueue<T>::size(void)const
{
    return writeCount_-readCount_;
}
}
#endif /* __AQUEUE_H__ */

