//
// Created by matei on 14.12.2018.
//

#ifndef SERVEROIO_CLIENT_ENTRY_H
#define SERVEROIO_CLIENT_ENTRY_H

#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FAT_ERROR(X) \
do\
{\
 perror("Eroare la "#X"().\n");\
 exit(-1);\
}while(false)
#define RET_ERROR(X,R) \
do\
{\
 perror("Eroare la "#X"().\n");\
 return ( R );\
}while(false)
#define ERROR(X) \
do\
{\
 perror("Eroare la "#X"().\n");\
}while(false)

using namespace std;

void create_path(const char* dirs);
void cpy_file(const char* from, const char* to);
void rm_r(const char* path);

template <class T>
void my_switch(T& a,T& b){
    T aux = a;
    a = b;
    b = aux;
}
#endif //SERVEROIO_CLIENT_ENTRY_H
