#include "rdp_config.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_thread.hpp"
#include <gtest/gtest.h>
using namespace rdp;
namespace {
struct Productor : public Thread
{
    RWQueue<int>& queue;
    Productor(RWQueue<int>& queue):queue(queue){}
    ~Productor(){ join(); }
    virtual void run(void)
    {
        int i = 0;
        while(i < 0xFFF) {
            int* p = this->queue.writeLock();
            *p = i;
            this->queue.writeUnlock();
            i += 7;
        }
    }
};
struct Consumer : public Thread
{
    RWQueue<int>& queue;
    Consumer(RWQueue<int>& queue):queue(queue){}
    ~Consumer(){ join(); }
    virtual void run(void)
    {
        int i =0;
        while(i < 0xFFF) {
            int* p;
            while(p = this->queue.readLock()) {
                EXPECT_EQ( *p, i);
                this->queue.readUnlock();
                i += 7;
            }
        }
    }
};
}
TEST(RWQueue, basic)
{
    RWQueue<int> queue;
    {
        Productor p(queue);
        Consumer c(queue);
        p.start();
        c.start();
    }
    EXPECT_EQ(queue.size(), 0);
}
