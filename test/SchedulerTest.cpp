#include <gtest/gtest.h>

#include "Scheduler.hpp"

class SchedulerTest : public ::testing::Test {
public:
    SchedulerTest() = default;
};

TEST_F(SchedulerTest, not_scheduled_when_empty) {
    Scheduler scheduler({});
    EXPECT_FALSE(scheduler.isScheduled(system_clock::now()));
}
