#pragma once

#include <chrono>
#include <list>

#include <ArduinoJson.h>
#include <date.h>
#include <iostream>
#include <sstream>

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

    Schedule(
        const char* start,
        seconds period,
        seconds duration)
        : Schedule(parseIsoDate(start), period, duration) {
    }

    Schedule(const JsonObject& json)
        : Schedule(
            json["start"].as<const char*>(),
            seconds { json["period"].as<int>() },
            seconds { json["duration"].as<int>() }) {
    }

    const time_point<system_clock> start;
    const seconds period;
    const seconds duration;

private:
    static time_point<system_clock> parseIsoDate(const char* value) {
        std::istringstream in(value);
        time_point<system_clock> date;
        in >> date::parse("%FT%TZ", date);
        return date;
    }
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
