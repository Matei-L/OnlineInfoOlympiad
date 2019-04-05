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



