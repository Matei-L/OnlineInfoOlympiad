//
// Created by matei on 15.12.2018.
//

#ifndef SERVEROIO_TESTER_H
#define SERVEROIO_TESTER_H

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/times.h>
#include <sys/time.h>
#include "Aux.h"
#include "Config.h"

void compute_grade(client_entry* my_client,Config* config);
bool check_test(FILE* to_test,FILE* ok);
#endif //SERVEROIO_TESTER_H
