#ifndef BANKS_TIME_H
#define BANKS_TIME_H

#include <time.h>

class Time {
public:
    static time_t current_time() {
        return current_time_;
    }

    static void step() {
        ++current_time_;
    }

    static time_t month_duration() {
        return 30;
    }

private:
    static time_t current_time_;
};


#endif //BANKS_TIME_H
