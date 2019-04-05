//
// Created by matei on 04.12.2018.
//

#include "Timer.h"

Timer::Timer() {
    bzero(&starting_moment, sizeof(timespec));
    nr_sec = 0;
    clk_id = CLOCK_REALTIME;
    expired = false;
}

Timer::Timer(clockid_t clk_id) {
    bzero(&starting_moment, sizeof(timespec));
    nr_sec = 0;
    this->clk_id = clk_id;
    expired = false;
}

void Timer::start(time_t nr_sec) {
    this->nr_sec = nr_sec;
    clock_gettime(clk_id,&starting_moment);
    expired = false;
}

int Timer::check() {
    if(!expired) {
        bzero(&current_moment, sizeof(timespec));
        clock_gettime(clk_id, &current_moment);
        time_t dif = current_moment.tv_sec - starting_moment.tv_sec;
        if (dif >= nr_sec) {
            expired = true;
            return -1;
        } else {
            return (int)(nr_sec-dif);
        }
    }
    else return -1;
}
