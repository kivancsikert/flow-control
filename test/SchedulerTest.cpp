#include <gtest/gtest.h>

#include "Scheduler.hpp"

class SchedulerTest : public ::testing::Test {
public:
    SchedulerTest() = default;
};

TEST_F(SchedulerTest, test) {
    EXPECT_EQ(constant, 1);
}
