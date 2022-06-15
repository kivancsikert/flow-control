#pragma once

#include <chrono>
#include <list>

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::time_point;

class Schedule {
public:
    Schedule(
        time_point<system_clock> start,
        seconds period,
        seconds duration)
        : start(start)
        , period(period)
        , duration(duration) {
    }

    const time_point<system_clock> start;
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
            if (time < schedule.start) {
                // Skip schedules that have not yet started
                continue;
            }
            auto offset = time - schedule.start;
            if (offset % schedule.period < schedule.duration) {
                return true;
            }
        }
        return false;
    }

private:
    const std::list<Schedule> schedules;
};
