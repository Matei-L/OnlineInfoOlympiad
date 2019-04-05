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
#include "Timer.h"

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

struct client_entry {   /* Datele unui client.*/
    char name[50] = "\0";   /* Nume concurent.*/
    char pass[25] = "\0";   /* Parola concurent.*/
    int socket = 0; /* Socket concurent.*/
    Timer timer;    /* Timer pentru rezolvarea problemelor.*/
    bool waiting_rez = false;   /* FLAG: a dat deja submit?*/
    bool thread_blocked = false;    /* FLAG: exista un thread ce se ocupa de
                                     * comunicarea cu clientul in acest moment?*/
    bool line_closed = true;    /* FLAG: concurentul este momentan offline?*/
    bool forced_to_submit = false;  /* FLAG: concurentul a fost fortat de server sa
                                     * trimita rezolvarile.*/
    float score = 0;    /* Punctajul clientului.*/
    string comments = "";   /* Info despre notarea clientului.*/
    int time = 0;  /* Numarul de secunde in care clientul a rezolvat cerintele.*/

};

void create_path(const char* dirs);
void cpy_file(const char* from, const char* to);
void rm_r(const char* path);

void sort_and_print(client_entry* clients,int num_clients, const char* file);
void sort(client_entry* clients,int num_clients);

template <class T>
void my_switch(T& a,T& b){
    T aux = a;
    a = b;
    b = aux;
}
#endif //SERVEROIO_CLIENT_ENTRY_H
