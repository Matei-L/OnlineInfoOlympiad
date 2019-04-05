//
// Created by matei on 15.12.2018.
//

#include "Tester.h"

void compute_grade(client_entry *my_client, Config *config) {
    char* files = strdup(config->files);
    // deschidem directorul cu rezolvarile concurentului
    string dirname("rez/");
    dirname+= my_client->name;
    dirname+= "/";

    // pentru fiecare problema i
    char* file = strtok(files," ");
    for (int i = 0; i < config->nr_prob; ++i) {
        file = basename(file);
        string source = dirname + file; // fisierul problemei
        string exe = source + ".exe";   // executabilul problemei

        my_client->comments+=to_string(i+1);
        my_client->comments+=": ";

        pid_t fiu;
        if((fiu = fork()) < 0) {
            ERROR(fork);
            my_client->comments+="\n";
            file = strtok(nullptr," ");
            continue;
        }
        if(fiu == 0){
            // fiu
            // compilam codul sursa
            close(2);
            execlp("g++","g++",source.c_str(),"-o",exe.c_str(), nullptr);
        }
        else{
            //tata
            // asteptam compilarea codului sursa
            waitpid(fiu, nullptr,0);
            if(access(exe.c_str(),F_OK) == -1){
                // nu exista fisierul
                my_client->comments+= "COMPILATION ERROR";
            }
            else{
                // testeaza solutia pe instantele din config serverului
                int num_tests = config->get_num_tests(i);
                float point = (float)100/num_tests;

                // pentru fiecare instanta j
                for (int j = 0; j < num_tests; ++j) {
                    tms times_start,times_stop;

                    /* copiem instanta j din zona de stocare a problemelor
                     * in folderul de notare a clientului my_client
                     * */
                    string aux;
                    aux = dirname+config->get_in(i);
                    cpy_file(config->get_test(i,j+1).c_str(),aux.c_str());

                    /*
                     * pregatim rularea sursei pe instanta j
                     */
                    aux = "./";
                    aux+= file;
                    aux+= ".exe";
                    times(&times_start);
                    if((fiu = fork()) < 0) {
                        ERROR(fork);
                        my_client->comments+="\n";
                        file = strtok(nullptr," ");
                        continue;
                    }
                    if(fiu == 0){
                        // fiu
                        /* setam o alarma pentru a indentifica
                         * cazurile de loop infinit sau pentru a opri
                         * solutiile care dureaza mult prea mult
                         * */
                        struct itimerval timer,timer_aux;
                        bzero(&timer,sizeof(timer));
                        double time_lim = config->get_time(i) * 1.2;
                        timer.it_value.tv_sec = (int)time_lim;
                        timer.it_value.tv_usec = (int)((time_lim-(int)time_lim)*1000000);
                        setitimer(ITIMER_PROF,&timer,&timer_aux);
                        /*
                         * rulam rezolvarea clientului
                         */
                        chdir(dirname.c_str());
                        execl(aux.c_str(),aux.c_str() , nullptr);
                    }
                    else{
                        // tata
                        // asteptam rularea testului
                        waitpid(fiu, nullptr,0);
                        times(&times_stop);
                        long double clktck=sysconf(_SC_CLK_TCK);

                        // verificam daca a fost generat fisierul .out dorit
                        aux = dirname + config->get_out(i);
                        if(access(aux.c_str(),F_OK) == -1){
                            // nu exista fisierul
                            my_client->comments+="WRONG/MISSING OUTPUT FILE";
                        }
                        else {
                            FILE *ok, *to_test;
                            // deschidem fisierul .out generat de rezolvarea clientului
                            if ((to_test = fopen(aux.c_str(), "r")) == nullptr) {
                                ERROR(fopen);
                                my_client->comments+="\n";
                                file = strtok(nullptr," ");
                                continue;
                            }
                            // deschidem fisierul .out corect
                            aux = config->get_test_rez(i, j + 1);
                            if ((ok = fopen(aux.c_str(), "r")) == nullptr) {
                                ERROR(fopen);
                                my_client->comments+="\n";
                                file = strtok(nullptr," ");
                                continue;
                            }
                            // Daca rezolvarea a avut eficienta timp dorita
                            if (((times_stop.tms_cstime - times_start.tms_cstime) +
                                 (times_stop.tms_cutime - times_start.tms_cutime)) / (double) clktck <=
                                config->get_time(i)) {
                                // Ii verificam corectitudinea
                                if (check_test(to_test, ok)) {
                                    // ii oferim punctul pentru aceasta instanta
                                    my_client->score += point;
                                    my_client->comments += "OK ";
                                } else {
                                    my_client->comments += "WRONG ";
                                }
                            } else {
                                my_client->comments += "T.L.E. ";
                            }
                            fclose(to_test);
                            fclose(ok);
                        }
                    }
                }
            }
        }
        my_client->comments+="\n";
        file = strtok(nullptr," ");
    }

    free(files);
}

bool check_test(FILE* to_test,FILE* ok) {
    size_t l_to_test=0,l_ok=0;
    char* buff_to_test= nullptr,*buff_ok= nullptr;
    char *saveptr1, *saveptr2;
    ssize_t rez1,rez2;
    // citim linie cu linie din cele doua fisiere
    rez1 = getline(&buff_to_test,&l_to_test,to_test);
    rez2 = getline(&buff_ok,&l_ok,ok);
    while( rez1 != -1 && rez2 != -1  ){
        // luam liniile token cu token
        char *token_to_test = strtok_r(buff_to_test," \n\r",&saveptr1);
        char *token_ok = strtok_r(buff_ok," \n\r",&saveptr2);

        while (token_ok != nullptr && token_to_test != nullptr){
            if(strcmp(token_ok,token_to_test) != 0){
                // daca am gasit doi tokeni diferiti rezolvarea este gresita
                free(buff_ok); buff_ok = nullptr;
                free(buff_to_test); buff_to_test = nullptr;
                l_to_test=0;
                l_ok=0;
                return false;
            }
            // trecem la urmatorul token
            token_to_test = strtok_r(nullptr," \n\r",&saveptr1);
            token_ok = strtok_r(nullptr," \n\r",&saveptr2);
        }
        free(buff_ok); buff_ok = nullptr;
        free(buff_to_test); buff_to_test = nullptr;
        l_to_test=0;
        l_ok=0;

        // trecem la urmatoarea linie
        rez1 = getline(&buff_to_test,&l_to_test,to_test);
        rez2 = getline(&buff_ok,&l_ok,ok);
    }
    // daca numarul de linii e diferit rezolvarea este gresita
    if(!(rez1 == -1 && rez2 == -1))
        return false;
    return true;
}
