//
// Created by matei on 05.12.2018.
//

#ifndef OIOPROT_OIOPROT_H
#define OIOPROT_OIOPROT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Aux.h"

/*
 * Protocolul de comunicare folosit pentru proiectul OIO
 * Toate mesajele sunt transmise cu ajutorul functiilor writemsg
 * si citite cu readmsg in format: nr_bytes bytes.
 * Functiile send/receiveCOMMAND sunt folosite pentru a trimite/
 * primi comenzile respective si datele de care trebuie insotite.
 * Toate functiile returneaza un numar >= 0 in caz de succes sau
 * -1 in cazul de eraoare.
 */
#define FILE_BUFF 1024

enum COMMAND: int{LOGIN,REJECT,ACCEPT,ALRIGHT,
    CONTEST_INFO,CONTEST_INFO_REQ,
    TIME,TIME_REQ,CONTEST_ANS,CONTEST_ANS_REQ,
    CONTEST_REZ,CONTEST_REZ_REQ,LOGOUT,CONTEST_BASIC_DATA,
    CONTEST_BASIC_DATA_REQ};

int writemsg(int to,const void* what,size_t length);
ssize_t readmsg(int from,void*& in);

int sendfile(int to,const char* file);
int receivefile(int from,const char* file);

int sendLOGIN(int to,char* name,char* pass,int code);
int receiveLOGIN(int from,char* name,char* pass,int& code);

int sendREJECT(int to,const char* reason);
int receiveREJECT(int from,char*& reason);

int sendACCEPT(int to);

int sendCONTEST_INFO(int to,int nr_sec,int nr_prob,const char* files);
int receiveCONTEST_INFO(int from,int& nr_sec,int& nr_prob,char*& files);

int sendCONTEST_INFO_REQ(int to);

int sendTIME(int to,int nr_sec);
int receiveTIME(int from,int& nr_sec);

int sendTIME_REQ(int to);

int sendCONTEST_ANS(int to,int nr_prob,const char* files);
int receiveCONTEST_ANS(int from,int nr_prob,const char *files,const char* prefix);

int sendCONTEST_ANS_REQ(int to);

int sendCONTEST_REZ(int to,const char* file);
int receiveCONTEST_REZ(int from,const char* file);

int sendCONTEST_REZ_REQ(int to);

int sendLOGOUT(int to);

int sendCONTEST_BASIC_DATA(int to,int nr_sec,int nr_prob,const char* files);
int receiveCONTEST_BASIC_DATA(int from,int& nr_sec,int& nr_prob,char*& files);

int sendCONTEST_BASIC_DATA_REQ(int to);

int sendALRIGHT(int to,const char* reason);
int receiveALRIGHT(int from,char*& reason);

int enable_keepalive(int with);
#endif //OIOPROT_OIOPROT_H
