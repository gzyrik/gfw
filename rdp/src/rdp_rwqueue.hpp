#ifndef __RDP_RWQUEUE_HPP__
#define __RDP_RWQUEUE_HPP__
namespace rdp {
template<typename T> class RWQueue
{
public:
    typedef T value_type;

    RWQueue (int initSize = 0);

    ~RWQueue();

    /** д��ʽ��ס����,���ؿ�д��λ��,
     * ����������� writeUnlock �� cancelWrite
     */
    T* writeLock(void);

    /** ����д�벢����
     * @param[in] cancelToLocation ������λ��
     */
    void cancelWrite(T* cancelToLocation);

    /** ���д����������� */
    void writeUnlock(void);

    /** ����ʽ��ס����,����,����0
     * @note �����ط�0, ������������ readUnlock �� cancelRead
     */
    T* readLock(void);

    /** ������ȡ������
     * @param[in] cancelToLocation ������λ��
     */
    void cancelRead(T* cancelToLocation);

    /** ��ɶ�ȡ���������� */
    void readUnlock(void);

    /** ���տ��еĽ�� */
    void shrink(void);

    /** ���ض��г��� */
    int size(void) const;

    /** ��Ӳ��� */
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
    dataPtr -> next = writePtr_;// ���ɻ��ε�������
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
    //[writeAheadPtr_, readPtr_)֮��Ľ���ǿ��е���Ч����
    volatile Node *node = writeAheadPtr_->next;
    while(node != readPtr_)
    {
        assert(node->readyToRead==false);
        volatile Node* next=node->next;
        delete node;
        node=next;
    }
    //�ٴν�����������
    writeAheadPtr_->next = readPtr_;
}
//-----------------------------------------------------------------------------
template<typename T> int RWQueue<T>::size(void)const
{
    return writeCount_-readCount_;
}
}
#endif /* __AQUEUE_H__ */

