#include <gtest/gtest.h>

#include "Scheduler.hpp"

using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

class SchedulerTest : public ::testing::Test {
public:
    SchedulerTest() = default;

    const time_point<system_clock> base { system_clock::now() };
};

TEST_F(SchedulerTest, not_scheduled_when_empty) {
    Scheduler scheduler({});
    EXPECT_FALSE(scheduler.isScheduled(base));
}

TEST_F(SchedulerTest, matches_single_schedule) {
    Scheduler scheduler({
        Schedule(base, minutes { 1 }, seconds { 15 }),
    });
    EXPECT_TRUE(scheduler.isScheduled(base));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 1 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 14 }));
    EXPECT_FALSE(scheduler.isScheduled(base + seconds { 15 }));
    EXPECT_FALSE(scheduler.isScheduled(base + seconds { 30 }));
    EXPECT_FALSE(scheduler.isScheduled(base + seconds { 59 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 60 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 74 }));
    EXPECT_FALSE(scheduler.isScheduled(base + seconds { 75 }));
}

TEST_F(SchedulerTest, matches_multiple_schedules) {
    Scheduler scheduler({
        Schedule(base, minutes { 1 }, seconds { 15 }),
        Schedule(base, minutes { 5 }, seconds { 60 }),
    });
    EXPECT_TRUE(scheduler.isScheduled(base));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 1 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 14 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 15 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 30 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 59 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 60 }));
    EXPECT_TRUE(scheduler.isScheduled(base + seconds { 74 }));
    EXPECT_FALSE(scheduler.isScheduled(base + seconds { 75 }));

    EXPECT_TRUE(scheduler.isScheduled(base + minutes { 2 }));
    EXPECT_TRUE(scheduler.isScheduled(base + minutes { 2 } + seconds { 1 }));
    EXPECT_TRUE(scheduler.isScheduled(base + minutes { 2 } + seconds { 14 }));
    EXPECT_FALSE(scheduler.isScheduled(base + minutes { 2 } + seconds { 15 }));
    EXPECT_FALSE(scheduler.isScheduled(base + minutes { 2 } + seconds { 30 }));
    EXPECT_FALSE(scheduler.isScheduled(base + minutes { 2 } + seconds { 59 }));
    EXPECT_TRUE(scheduler.isScheduled(base + minutes { 2 } + seconds { 60 }));
    EXPECT_TRUE(scheduler.isScheduled(base + minutes { 2 } + seconds { 74 }));
    EXPECT_FALSE(scheduler.isScheduled(base + minutes { 2 } + seconds { 75 }));
}
