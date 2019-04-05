//
// Created by matei on 05.12.2018.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include "OIOprot.h"

using namespace std;

/* date legate de server
 * */
char adr_server[16];
int port_commands;
int sock_server = 0;

/* date despre clientul nostru
 * */
char name[50],pass[25];
bool logged_in = false;

/* date despre sesiunea curenta
 * */
int code;
int nr_sec = 0,nr_prob = 0;
char* files = nullptr;

void sig_handler(int signo); /* handler de semnale ce va inchide clientul in mod corect */
void server_offline(); /* inchide clientul daca serverul a fost inchis */
void show_info(); /* afiseaza pe ecran datele despre concurs primite de la server */
void receive_server_COMMAND(int sock_server); /* citeste si trateaza comanda primita de la server */
void receive_user_COMMAND(int to); /* citeste si trateaza comanda primita de la tastatura */

int main (int argc, char *argv[])
{
    /* prindem toate semnalele de inchidere a procesului
     * cu exceptia lui SIGKILL in sig_handler
     * */
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        FAT_ERROR(signal);
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        FAT_ERROR(signal);
    if (signal(SIGQUIT, sig_handler) == SIG_ERR)
        FAT_ERROR(signal);
    if (signal(SIGHUP, sig_handler) == SIG_ERR)
        FAT_ERROR(signal);

    /* citim unde trebuie sa ne conectam
     * */
    printf("Adresa serverului: ");
    scanf("%s",adr_server);
    printf("Portul serverului: ");
    scanf("%d",&port_commands);

    /* deschidem un socket in protocol TCP
     * */
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(adr_server);
    server.sin_port = htons((uint16_t)port_commands);

    if ((sock_server = socket(AF_INET, SOCK_STREAM,0)) == -1)
        FAT_ERROR(socket);

    /* conectam socket-ul la server
     * */
    if (connect(sock_server,(struct sockaddr*)&server,sizeof(struct sockaddr)) == -1)
        FAT_ERROR(connect);

    /* citim informatiile de login
     * */
    printf("\nlogin\n");
    printf("Nume cont: ");
    scanf("%s", name);
    printf("Pass: ");
    scanf("%s", pass);
    printf("Cod concurs: ");
    scanf("%d", &code);

    /* trimitem informatiile de login
     * */
    if( sendLOGIN(sock_server, name, pass, code) == -1 ){
        server_offline();
    }

    /* citim raspunsul serverului pentru login
     * */
    receive_server_COMMAND(sock_server);

    /* logarea nu a reusit si inchidem procesul
     * */
    if(!logged_in){
        close (sock_server);
        return 0;
    }

    /* pregatim datele necesare primitivei select
     * */
    int nfds = sock_server;
    fd_set read_fds,followed_fds;
    FD_ZERO(&followed_fds);     /* initializam lista de descriptori */
    FD_SET(sock_server,&followed_fds);  /* adaugam socket-ul spre server in lista */
    FD_SET(0,&followed_fds);    /* adaugam 0 (citirea de la tastatura) in lista */
    struct timeval select_time;
    select_time.tv_sec = 0;
    select_time.tv_usec = 100000;

    /* cat timp suntem conectati
     * */
    while(logged_in){
        /* pregatim apelul primitivei select
         * */
        select_time.tv_sec = 0;
        select_time.tv_usec = 100000;
        bcopy(&followed_fds,&read_fds,sizeof(fd_set));

        if(select(nfds+1,&read_fds, nullptr, nullptr,&select_time)<0)
            FAT_ERROR(select);

        /* tratam comenzile de la server
         * */
        if(FD_ISSET(sock_server,&read_fds)){
            receive_server_COMMAND(sock_server);
        }
        /* tratam comenzile de la tastatura
         * */
        if(FD_ISSET(0,&read_fds)){
            receive_user_COMMAND(sock_server);
        }
    }

    /* inchidem programul
     * */
    FD_CLR(sock_server,&read_fds);
    FD_CLR(0,&read_fds);
    free(files);
    close (sock_server);
    return 0;
}


void receive_server_COMMAND(int sock_server)
{
    /* citim comanda de la server
     * */
    COMMAND command;
    void *pVoid = &command;
    if( readmsg(sock_server,pVoid) == -1 ){
        server_offline();
    }

    /* tratam comanda in functie de tipul acesteia
     * */
    switch(command)
    {
        case REJECT: {
            char *reason = nullptr;
            if( receiveREJECT(sock_server, reason) == -1 ){
                server_offline();
            }
            printf("\nComanda refuzata: %s\n", reason);
            free(reason);
            break;
        }
        case ALRIGHT: {
            char *msg = nullptr;
            if( receiveALRIGHT(sock_server, msg) == -1 ){
                server_offline();
            }
            printf("\nALRIGHT: %s\n", msg);
            free(msg);
            break;
        }
        case ACCEPT: {
            logged_in = true;
            break;
        }
        case CONTEST_INFO:{
            int nr_sec_aux,nr_prob_aux;
            if( receiveCONTEST_INFO(sock_server,nr_sec_aux,nr_prob_aux,files) == -1 ){
                server_offline();
            }
            if(nr_sec == 0 && nr_prob == 0){
                nr_sec = nr_sec_aux;
                nr_prob = nr_prob_aux;
            }
            show_info();
            break;
        }
        case CONTEST_BASIC_DATA:{
            int nr_sec_aux,nr_prob_aux;
            if( receiveCONTEST_BASIC_DATA(sock_server,nr_sec_aux,nr_prob_aux,files) == -1 ){
                server_offline();
            }
            if(nr_sec == 0 && nr_prob == 0){
                nr_sec = nr_sec_aux;
                nr_prob = nr_prob_aux;
            }
            show_info();
            break;
        }
        case TIME:{
            int nr_sec;
            if( receiveTIME(sock_server,nr_sec) == -1 ){
                server_offline();
            }
            printf("\nMai sunt: %dh %dm %ds\n",nr_sec/3600,(nr_sec%3600)/60,nr_sec%60);
            break;
        }
        case CONTEST_ANS_REQ:{
            if( sendCONTEST_ANS(sock_server,nr_prob,files) == -1 ){
                server_offline();
            }
            break;
        }
        case CONTEST_REZ:{
            if( receiveCONTEST_REZ(sock_server,"Rez.txt") == -1 ){
                server_offline();
            }
            break;
        }
        default: {
            printf("\nAm primit o comanda ce nu ma priveste: %d\n",(int)command);
            break;
        }
    }
}

void receive_user_COMMAND(int to)
{
    /* citim comanda de la server
     * */
    size_t n = 0;
    char *line = nullptr;
    if(getline(&line,&n,stdin) == -1) {
        /* afisam eroarea */
        free(line);
        ERROR(getline);
        /* inchidem clientul */
        sendLOGOUT(sock_server);
        logged_in = false;
        free(files);
        close (sock_server);
        exit(0);
    }
    /* eliminam \n de la final
     * */
    n = strlen(line);
    if(line[n-1] == '\n')line[n-1] = '\0';

    /* daca linia citita este una din comenzile cunoscute clientului
     * atunci o tratam, daca nu, nu facem nimic */
    if(strcmp(line,"logout") == 0 || strcmp(line,"exit") == 0){
        if( sendLOGOUT(to) == -1 ){
            server_offline();
        }
        logged_in = false;
    }else if(strcmp(line,"time") == 0){
        if( sendTIME_REQ(to) == -1 ){
            server_offline();
        }
    }else if(strcmp(line,"submit") == 0 || strcmp(line,"send") == 0){
        if( sendCONTEST_ANS(to,nr_prob,files) == -1 ){
            server_offline();
        }
    }else if(strcmp(line,"info") == 0){
        if( sendCONTEST_INFO_REQ(to) == -1 ){
            server_offline();
        }
    }else if(strcmp(line,"rez") == 0){
        if( sendCONTEST_REZ_REQ(to) == -1 ){
            server_offline();
        }
    }else if(strcmp(line,"basic info") == 0){
        if( sendCONTEST_BASIC_DATA_REQ(to) == -1 ){
            server_offline();
        }
    }

    free(line);
}
void show_info(){
    printf("Fisierele concursului:\n%s\n"
           "Durata concursului: %dh %dm %ds\n"
           "Aveti de rezolvat un numar de %d probleme.\n"
           "Cerintele acestor probleme sunt in fisierele \".cpp\" de mai sus.\n"
           "Rezolvati fiecare problema in \".cpp\" acesteia.\n"
           "Nu mutati sau stergeti fisierele \".cpp\" sau directoarele in care se afla acestea.\n"
           "Daca nu ati trimis rezolvarile prin intermediul comenzii \"submit\" aveti grija sa fiti logati "
           "cand se termina timpul acordat rezolvarii problemelor sau rezolvarile voastre nu vor mai fi luate in calcul.\n"
           "Succes!\n",
           files,nr_sec/3600,(nr_sec%3600)/60,nr_sec%60,nr_prob);
}
void sig_handler(int signo)
{
    /* anuntam serverul de faptul ca n-am deconectat
     * */
    sendLOGOUT(sock_server);
    /* eliberam memoria alocata si inchidem conexiunea
     * */
    logged_in = false;
    free(files);
    close (sock_server);
    exit(0);
}

void server_offline(){
    printf("\nServer offline!\n");
    logged_in = false;
    free(files);
    close (sock_server);
    exit(0);
}