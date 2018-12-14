#include "rdp_ringbuffer.hpp"
#include <gtest/gtest.h>
#include <gtest/gtest.h>
using namespace rdp;
TEST(RingBuffer, PushbackOverflow)
{
    RingBuffer<int> rb(12);
    int tail, head;
    srand(static_cast<unsigned>(Time::now().millisec()));
    for(head=tail=0; tail<10000; ++tail) {
        if (rb.size() > 0 && rand() > RAND_MAX/2) {
            EXPECT_EQ(rb.front(), head);
            head++;
            rb.pop_front();
        }
        rb.push_back(tail);
        EXPECT_EQ(rb.back(), tail);
    }
    EXPECT_EQ(rb.size(), tail-head);
    for(int i=head;i<tail;++i){
        EXPECT_EQ(rb.front(), i);
        rb.pop_front();
    }
    EXPECT_EQ(rb.size(), 0);
}
