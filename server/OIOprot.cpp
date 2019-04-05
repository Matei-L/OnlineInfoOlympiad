//
// Created by matei on 05.12.2018.
//

#include "OIOprot.h"

int writemsg(int to,const void* what,size_t length)
{
    if(what != nullptr)
    {
        char buff_len[sizeof(size_t)];
        bcopy(&length,buff_len, sizeof(size_t));

        ssize_t n;
        size_t sum = 0;
        while(sizeof(size_t)-sum>0) { // trimitem dimensiunea mesajului
            n = write(to,buff_len+sum,sizeof(size_t)-sum);
            if (n <= 0){ // conexunia a picat sau celalalt capat a apelat close()
                if( errno ==  ECONNRESET || errno == ENETDOWN || errno == ENETUNREACH )
                    return -1;
                else RET_ERROR(write,-1);
            }
            sum+=n;
        }

        sum = 0;
        while(length-sum>0) { // trimitem mesajul
            n = write(to,(const char*)what+sum,length-sum);
            if (n <= 0){ // conexunia a picat sau celalalt capat a apelat close()
                if( errno ==  ECONNRESET || errno == ENETDOWN || errno == ENETUNREACH )
                    return -1;
                else RET_ERROR(write,-1);
            }
            sum+=n;
        }
    }
    return 0;
}

ssize_t readmsg(int from,void*& in)
{
    size_t length;
    char buff_len[sizeof(size_t)];
    bcopy(&length,buff_len, sizeof(size_t));

    ssize_t n;
    size_t sum = 0;
    while(sizeof(size_t)-sum>0) { // citim cat de lung va fi mesajul
        n = read(from,buff_len+sum,sizeof(size_t)-sum);
        if (n <= 0) { // conexunia a picat sau celalalt capat a apelat close()
            if(n == 0){ // a fost apelat close()
                return  -1;
            }
            else RET_ERROR(read,-1);
        }
        sum+=n;
    }

    length = *((size_t*)buff_len);

    if(in == nullptr)in = malloc(length); /* alocam spatiu suficient pentru stocarea mesajului
                                           * acest lucru are loc doar cand in == nullptr, iar
                                           * spatiul alocat astfel va fi eliberat prin apelul
                                           * functiei free() de catre cine apeleaza readmsg
                                           */

    sum = 0;
    while(length-sum>0) { // citim mesajul
        n = read(from,(char*)in+sum,length-sum);
        if (n <= 0) { // conexunia a picat sau celalalt capat a apelat close()
            free(in);
            if(n == 0){ // a fost apelat close()
                return  -1;
            }
            else RET_ERROR(read,-1);
        }
        sum+=n;
    }

    return (ssize_t)length;
}

int sendfile(int to,const char *file) {
    FILE * g;
    // deschidem pentru citire
    if((g = fopen(file,"r")) == nullptr)
        ERROR(fopen);
    else {
        // trimitem cate FILE_BUFF bytes
        char buff[FILE_BUFF];
        while (fgets(buff, FILE_BUFF, g) != nullptr) {
            if (writemsg(to, buff, (strlen(buff) + 1) * sizeof(char)) == -1) {
                if( g != nullptr ) fclose(g);
                return -1;
            }
        }
    }
    // trimitem un mesaj individual de dimensiune 1
    // ce va contine '\0' pentru a anunta sfarsitul transmiterii
    char eof = '\0';
    if ( writemsg(to,&eof, sizeof(char)) == -1 ){
        if( g != nullptr ) fclose(g);
        return -1;
    }

    if( g != nullptr ) fclose(g);
    return 0;
}

int receivefile(int from,const char *file) {
    FILE *g;
    size_t length = strlen(file);
    char* temp = (char*)malloc(length + 1);
    // daca este nevoie de crearea a noi directoare
    // pentru a salva fisierul, le cream
    bcopy(file,temp,length+1);
    create_path(dirname(temp));
    free(temp);
    // deschidem pentru scriere
    if((g = fopen(file,"w")) == nullptr)
        ERROR(fopen);
    // citim continutul fisierului mesaj cu mesaj
    char *buff = (char*)malloc(FILE_BUFF);
    do{
        void *pVoid = buff;
        ssize_t slength = readmsg(from,pVoid);
        if(slength < 0) {
            free(buff);
            if( g != nullptr ) fclose(g);
            return -1;
        }
        length = (size_t) slength;
        buff = (char*)pVoid;
        if(!(length == sizeof(char) && *buff == '\0'))
            fprintf(g,"%s",buff);
    }while(!(length == sizeof(char) && *buff == '\0')); // pana intalnim mesajul de oprire

    free(buff);
    if( g != nullptr ) fclose(g);
    return 0;
}

int sendLOGIN(int to, char *name, char *pass, int code) {
    COMMAND login = LOGIN;
    // anuntam tipul de comanda
    if ( writemsg(to,&login,sizeof(COMMAND)) == -1){
        return -1;
    }
    // trimitem datele in mesaje individuale
    if ( writemsg(to,name,(strlen(name) + 1)* sizeof(char)) == -1 ){
        return -1;
    }
    if ( writemsg(to,pass,(strlen(pass) + 1)* sizeof(char)) == -1 ){
        return -1;
    }
    if ( writemsg(to,&code,sizeof(int)) == -1 ){
        return -1;
    }

    return 0;
}

int receiveLOGIN(int from, char *name, char *pass, int &code) {
    // tipul mesajului a fost deja citit, mai trebuie citite datele
    // citim numele
    void *pVoid = name;
    if( readmsg(from,pVoid) == -1 ){
        return  -1;
    }
    name = (char*)pVoid;
    // citim parola
    pVoid = pass;
    if( readmsg(from,pVoid) == -1 ){
        return  -1;
    }
    pass = (char*)pVoid;
    // citim codul concursului
    pVoid = &code;
    if( readmsg(from,pVoid) == -1 ){
        return  -1;
    }

    return 0;
}

int sendREJECT(int to,const char *reason) {
    COMMAND reject = REJECT;
    if( writemsg(to,&reject,sizeof(COMMAND)) == -1 ){
        return -1;
    }

    if( writemsg(to,reason,(strlen(reason) + 1)* sizeof(char)) == -1 ){
        return -1;
    }
    return 0;
}

int receiveREJECT(int from, char*& reason) {
    void *pVoid = reason;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }
    reason = (char*)pVoid;
    return 0;
}

int sendACCEPT(int to) {
    COMMAND accept = ACCEPT;
    if( writemsg(to,&accept,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int sendCONTEST_INFO_REQ(int to) {
    COMMAND info_req = CONTEST_INFO_REQ;
    if( writemsg(to,&info_req,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int sendTIME(int to, int nr_sec) {
    COMMAND time = TIME;
    if( writemsg(to,&time,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    if( writemsg(to,&nr_sec,sizeof(int)) == -1 ){
        return -1;
    }
    return 0;
}

int receiveTIME(int from, int &nr_sec) {
    void *pVoid = &nr_sec;
    if( readmsg(from, pVoid) == -1){
        return -1;
    }
    return 0;
}

int sendTIME_REQ(int to) {
    COMMAND time_req = TIME_REQ;
    if( writemsg(to,&time_req,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int sendCONTEST_ANS_REQ(int to) {
    COMMAND ans_req = CONTEST_ANS_REQ;
    if( writemsg(to,&ans_req,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int sendCONTEST_REZ_REQ(int to) {
    COMMAND rez_req = CONTEST_REZ_REQ;
    if( writemsg(to,&rez_req,sizeof(COMMAND)) ==-1 ){
        return -1;
    }
    return 0;
}

int sendCONTEST_INFO(int to,int nr_sec,int nr_prob,const char* files) {
    COMMAND contest_info = CONTEST_INFO;
    if( writemsg(to,&contest_info,sizeof(COMMAND)) == -1 ){
        return -1;
    }

    if( writemsg(to,&nr_sec, sizeof(int)) == -1 ){
        return -1;
    }
    if( writemsg(to,&nr_prob, sizeof(int)) == -1 ){
        return -1;
    }
    if( writemsg(to,files,(strlen(files) + 1)* sizeof(char)) == -1 ){
        return -1;
    }

    size_t length = strlen(files);
    char* temp = (char*)malloc(length + 1);
    bcopy(files,temp,length+1);
    char *saveptr;

    char* file = strtok_r(temp," ",&saveptr);
    while(file){
        if( sendfile(to,file) == -1 ){
            free(temp);
            return -1;
        }
        file = strtok_r(nullptr," ",&saveptr);
    }

    free(temp);
    return 0;
}

int receiveCONTEST_INFO(int from,int& nr_sec,int& nr_prob,char*& files) {
    void* pVoid = &nr_sec;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }

    pVoid = &nr_prob;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }

    if(files != nullptr) {
        free(files);
        files = nullptr;
    }
    pVoid = files;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }
    files = (char*)pVoid;

    size_t length = strlen(files);
    char* temp = (char*)malloc(length + 1);
    bcopy(files,temp,length+1);
    char* saveptr;

    char* file = strtok_r(temp," ",&saveptr);
    while(file){
        if( receivefile(from,file) == -1){
            free(temp);
            return -1;
        }
        file = strtok_r(nullptr," ",&saveptr);
    }

    free(temp);
    return 0;
}

int sendCONTEST_ANS(int to, int nr_prob, const char *files) {
    COMMAND contest_ans = CONTEST_ANS;
    if( writemsg(to,&contest_ans,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    size_t length = strlen(files);
    char* temp = (char*)malloc(length + 1);
    bcopy(files,temp,length+1);
    char* saveptr;

    char* file = strtok_r(temp," ",&saveptr);
    for (int i = 0; i < nr_prob; ++i) {
        if( sendfile(to,file) == -1 ){
            free(temp);
            return -1;
        }
        file = strtok_r(nullptr," ",&saveptr);
    }

    free(temp);
    return 0;
}

int receiveCONTEST_ANS(int from, int nr_prob,const char *files, const char *prefix) {
    size_t length = strlen(files);
    char* temp = (char*)malloc(length + 1);
    bcopy(files,temp,length+1);
    char* saveptr;

    char* file = strtok_r(temp," ",&saveptr);
    for (int i = 0; i < nr_prob; ++i) {
        file = basename(file);
        char* new_file = (char*)malloc(strlen(prefix)+strlen(file)+2);
        sprintf(new_file,"%s/%s",prefix,file);
        if( receivefile(from,new_file) == -1 ){
            free(temp);
            free(new_file);
            return -1;
        }
        free(new_file);
        file = strtok_r(nullptr," ",&saveptr);
    }

    free(temp);
    return 0;
}

int sendCONTEST_REZ(int to, const char *file) {
    COMMAND contest_rez = CONTEST_REZ;
    if( writemsg(to,&contest_rez,sizeof(COMMAND)) == -1 ){
        return -1;
    }

    if( sendfile(to,file) == -1 ){
        return -1;
    }
    return 0;
}

int receiveCONTEST_REZ(int from,const char* file) {
    if( receivefile(from,file) == -1){
        return -1;
    }
    return 0;
}

int sendLOGOUT(int to) {
    COMMAND logout = LOGOUT;
    if( writemsg(to,&logout,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int sendCONTEST_BASIC_DATA(int to, int nr_sec, int nr_prob,const char* files) {
    COMMAND contest_basic_data = CONTEST_BASIC_DATA;
    if( writemsg(to,&contest_basic_data,sizeof(COMMAND)) == -1 ){
        return -1;
    }

    if( writemsg(to,&nr_sec, sizeof(int)) == -1 ){
        return -1;
    }
    if( writemsg(to,&nr_prob, sizeof(int)) == -1 ){
        return -1;
    }
    if( writemsg(to,files,(strlen(files) + 1)* sizeof(char)) == -1 ){
        return -1;
    }
    return 0;
}

int receiveCONTEST_BASIC_DATA(int from, int &nr_sec, int &nr_prob,char*& files) {
    void* pVoid = &nr_sec;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }

    pVoid = &nr_prob;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }

    if(files != nullptr) {
        free(files);
        files = nullptr;
    }
    pVoid = files;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }
    files = (char*)pVoid;
    return 0;
}

int sendALRIGHT(int to, const char *reason) {
    COMMAND alright = ALRIGHT;
    if( writemsg(to,&alright,sizeof(COMMAND)) == -1 ){
        return -1;
    }

    if( writemsg(to,reason,(strlen(reason) + 1)* sizeof(char)) == -1 ){
        return -1;
    }
    return 0;
}

int receiveALRIGHT(int from, char *&reason) {
    void *pVoid = reason;
    if( readmsg(from,pVoid) == -1 ){
        return -1;
    }
    reason = (char*)pVoid;
    return 0;
}

int sendCONTEST_BASIC_DATA_REQ(int to) {
    COMMAND basic_data_req = CONTEST_BASIC_DATA_REQ;
    if( writemsg(to,&basic_data_req,sizeof(COMMAND)) == -1 ){
        return -1;
    }
    return 0;
}

int enable_keepalive(int with){
    int opt_val = 1; // activam optiunea keepalive
    if(setsockopt(with, SOL_SOCKET, SO_KEEPALIVE,&opt_val,sizeof(opt_val)) == -1)
        return -1;

    opt_val = 60; // incepem trimiterea mesajelor dupa 60 de sec de inactivitate
    if(setsockopt(with, IPPROTO_TCP, TCP_KEEPIDLE, &opt_val, sizeof(int)) == -1)
        return -1;

    opt_val = 12;// trimitem cate unul la 12 secunde
    if(setsockopt(with, IPPROTO_TCP, TCP_KEEPINTVL, &opt_val, sizeof(int)) == -1)
        return -1;

    opt_val = 10;// odata ce n-au fost raspunse 10 la rand se va considera celalalt partener ca fiind picat
    if(setsockopt(with, IPPROTO_TCP, TCP_KEEPCNT, &opt_val, sizeof(int)) == -1)
        return -1;

    return 0;
}
