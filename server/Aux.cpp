//
// Created by matei on 14.12.2018.
//

#include "Aux.h"

void create_path(const char* dirs)
{
    size_t length = strlen(dirs);
    char* temp = (char*)malloc(length + 1);
    bcopy(dirs,temp,length+1);

    if(temp[length - 1] == '/')
        temp[length - 1] = '\0';

    for(char* p = temp + 1; *p; p++)
        if(*p == '/') {
            *p = '\0';
            mkdir(temp, S_IRWXU);
            *p = '/';
        }
    mkdir(temp, S_IRWXU);

    free(temp);
}

void cpy_file(const char *from, const char *to) {
    pid_t fiu;
    if((fiu = fork()) < 0) {
        ERROR(fork);
        return;
    }
    if(fiu == 0){
        // fiu
        execlp("cp", "cp", from, to, nullptr);
    }
    else{
        // tata
        waitpid(fiu, nullptr,0);
        if(access(to,F_OK) == -1){
            // nu exista fisierul
            ERROR(cp);
        }
    }
}

void rm_r(const char *path) {

    if(access(path,F_OK) != -1){
        pid_t fiu;
        if((fiu = fork()) < 0) {
            ERROR(fork);
            return;
        }
        if(fiu == 0){
            // fiu
            execlp("rm", "rm", "-r", path, nullptr);
        }
        else{
            // tata
            waitpid(fiu, nullptr,0);
            if(access(path,F_OK) != -1){
                // exista fisierul
                ERROR(rm_r);
            }
        }
    }
}

void sort_and_print(client_entry *clients, int num_clients, const char *file) {
    FILE* out = fopen(file,"w");
    sort(clients,num_clients);
    for(int i = 0; i < num_clients; ++i){
        fprintf(out,"%d) %s %.2f %dh %dm %ds\n %s",i+1,clients[i].name,clients[i].score,clients[i].time/3600,
                (clients[i].time%3600)/60,clients[i].time%60,clients[i].comments.c_str());
    }
    fclose(out);
}

void sort(client_entry *clients, int num_clients) { // insert sort dupa scor si viteza rezolvarii
    for (int i = 0; i < num_clients - 1; ++i) {
        int index_max = i;
        for (int j = i+1; j < num_clients; ++j) {
            if(clients[index_max].score < clients[j].score ||
                    (clients[index_max].score == clients[j].score &&
                    clients[index_max].time > clients[j].time)){
                index_max = j;
            }
        }
        if(index_max != i){
            my_switch(clients[index_max],clients[i]);
        }
    }
}



