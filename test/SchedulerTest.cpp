#include <gtest/gtest.h>

#include "Scheduler.hpp"

using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

class SchedulerTest : public ::testing::Test {
public:
    SchedulerTest() = default;

    const time_point<system_clock> base { system_clock::now() };
};

TEST_F(SchedulerTest, can_create_schedule) {
    Schedule schedule(base, hours { 1 }, minutes { 1 });
    EXPECT_EQ(schedule.start, base);
    EXPECT_EQ(schedule.period, hours { 1 });
    EXPECT_EQ(schedule.duration, minutes { 1 });
}

TEST_F(SchedulerTest, can_create_schedule_from_string) {
    Schedule schedule("2020-01-01T00:00:00Z", hours { 1 }, minutes { 1 });
    EXPECT_EQ(schedule.start, time_point<system_clock> { system_clock::from_time_t(1577836800) });
    EXPECT_EQ(schedule.period, hours { 1 });
    EXPECT_EQ(schedule.duration, minutes { 1 });
}

TEST_F(SchedulerTest, can_create_schedule_from_json) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, R"({
        "start": "2020-01-01T00:00:00Z",
        "period": 60,
        "duration": 15
    })");
    Schedule schedule(doc.as<JsonObject>());
    EXPECT_EQ(schedule.start, time_point<system_clock> { system_clock::from_time_t(1577836800) });
    EXPECT_EQ(schedule.period, minutes { 1 });
    EXPECT_EQ(schedule.duration, seconds { 15 });
}

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

TEST_F(SchedulerTest, does_not_match_schedule_not_yet_started) {
    Scheduler scheduler({
        Schedule(base, minutes { 1 }, minutes { 1 }),
    });
    EXPECT_FALSE(scheduler.isScheduled(base - seconds { 1 }));
    EXPECT_TRUE(scheduler.isScheduled(base));
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
