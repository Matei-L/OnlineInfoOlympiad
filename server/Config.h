//
// Created by matei on 08.12.2018.
//

#ifndef SERVEROIO_SERVER_CONFIGS_H
#define SERVEROIO_SERVER_CONFIGS_H

#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include "nlohmann/json.hpp"
#include <random>
#include "Timer.h"
#include "Aux.h"

using json = nlohmann::json;
using namespace std;

class Config {
    json config;
    json problems;
    json contenders;
public:
    int code;
    int nr_prob;
    int nr_sec_contest;
    int nr_contenders;
    int nr_sec_wait;
    char* files;

    Config();
    ~Config();

    bool search_client(client_entry client);
    int get_num_tests(int prob);
    string get_test(int prob,int index);
    string get_test_rez(int prob,int index);
    string get_in(int prob);
    string get_out(int prob);
    string get_path(int prob);
    string get_name(int prob);
    double get_time(int prob);
};

#endif //SERVEROIO_SERVER_CONFIGS_H
