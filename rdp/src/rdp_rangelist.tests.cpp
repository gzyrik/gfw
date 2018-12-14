#include "rdp_config.hpp"
#include "rdp_rangelist.hpp"
#include <gtest/gtest.h>
using namespace rdp;

TEST(RangeList, Insert_9_TO_6_10)
{
    RangeList<uint32_t> ranges;
    ranges.insert(6);
    ranges.insert(10);
    ranges.insert(9);
    EXPECT_EQ(ranges.size(), 2);
    EXPECT_EQ(ranges[0].first, 6);
    EXPECT_EQ(ranges[1].first, 9);
    EXPECT_EQ(ranges[1].second, 10);
}

TEST(RangeList, RandInsert_0_1000)
{
    const uint32_t num_val = 1000;
    std::vector<uint32_t> seeds;
    for(int i=0;i<num_val;++i)
        seeds.push_back(i);
    RangeList<uint32_t> ranges;
    while(seeds.size() > 1) {
        uint32_t& val = seeds[rand() % seeds.size()];
        ranges.insert(val);
        val = seeds.back();
        seeds.pop_back();
    }
    uint32_t& val = seeds[0];
    if (val == 0) {
        EXPECT_EQ(ranges.size(), 1);
        EXPECT_EQ(ranges[0].first, 1);
        EXPECT_EQ(ranges[0].second, num_val);
    }
    else if (val == num_val-1) {
        EXPECT_EQ(ranges.size(), 1);
        EXPECT_EQ(ranges[0].first, 0);
        EXPECT_EQ(ranges[0].second, num_val-2);
    }
    else {
        EXPECT_EQ(ranges.size(), 2);
        EXPECT_EQ(ranges[0].first, 0);
        EXPECT_EQ(ranges[0].second, val-1);
        EXPECT_EQ(ranges[1].first, val+1);
        EXPECT_EQ(ranges[1].second, num_val-1);
    }
    ranges.insert(val);
    EXPECT_EQ(ranges.size(), 1);
    EXPECT_EQ(ranges[0].first, 0);
    EXPECT_EQ(ranges[0].second, num_val-1);
}
