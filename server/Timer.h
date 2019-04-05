//
// Created by matei on 04.12.2018.
//

#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H
#include <time.h>
#include <strings.h>

class Timer {
    timespec starting_moment;
    timespec current_moment;
    time_t nr_sec;
    clockid_t clk_id;
    bool expired;
public:
    Timer();
    Timer(clockid_t clk_id);
    void start(time_t nr_sec); // porneste un timer de nr_sec secunde
    int check();    // intoarce -1 cand timpul a expirat sau nr de secunde ramase, altfel
};


#endif //TIMER_TIMER_H
