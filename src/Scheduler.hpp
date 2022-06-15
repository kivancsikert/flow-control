#pragma once

#include <chrono>
#include <list>

using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

class Schedule {
public:
    Schedule(
        time_point<system_clock> base,
        seconds period,
        seconds duration)
        : base(base)
        , period(period)
        , duration(duration) {
    }

    const time_point<system_clock> base;
    const seconds period;
    const seconds duration;
};

class Scheduler {
public:
    Scheduler(std::list<Schedule> schedules)
        : schedules(schedules) {
    }

    bool isScheduled(time_point<system_clock> time) {
        for (auto& schedule : schedules) {
            auto offset = time - schedule.base;
            if (offset.count() % schedule.period.count() < schedule.duration.count()) {
                return true;
            }
        }
        return false;
    }

private:
    const std::list<Schedule> schedules;
};
