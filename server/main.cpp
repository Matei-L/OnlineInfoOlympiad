//
// Created by matei on 04.12.2018.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "Timer.h"
#include "OIOprot.h"
#include "Config.h"
#include "Aux.h"
#include "Tester.h"

using namespace std;

/* Date legate de deschiderea socket-ului de bind
 */
int port_commands;
int sock_connect;

/* Date despre concursul curent
 */
Config* config;
client_entry *clients; // vectorul de participanti

/* Date despre starea concursului
 */
int num_connected_clients;
int num_waiting_clients;
bool accepting_rez_req = false;

/* Timer pentru logarea clientilor
 */
Timer* login_timer;
/* semafor pentru logarea clientilor
 */
sem_t mutex_login;

void sig_handler(int signo); /* handler de semnale ce va inchide server-ul in mod corect */
static void* serve_client(void* data);  /* citeste si trateaza comanda clientului.
                                         * data este de tip *client_entry.
                                         * va fi functie data thread-urilor.
                                         * */
static void* ans_req(void* data); /* trimite ANS_REQ clientului.
                                   * data este de tip *client_entry.
                                   * va fi functie data thread-urilor.
                                   */
client_entry* login(client_entry* my_client); /* verifica daca my_client se poate autentifica, sau nu.
 *                                             * in caz afirmativ il adauga / updateaza in vectorul de participanti
 *                                             * altfe, va returna nullptr si incheie comunicarea cu my_client.
 *                                             */
void logout(client_entry* my_client); /* incheie comunicatia cu clientul my_client */

int main()
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

    /* stergem fisierele / directoarele ramase de la rulari precedente*/
    rm_r("rez/");
    rm_r("Rez.txt");
    rm_r("trash/");

    /* configurez serverul pe baza fisierelor de cofigurare */
    try {
        config = new Config;
    }
    catch (...){
        perror("Eroare la configurarea serverului. Verificati fisierele de configurare.\n");
        delete config;
        exit(1);
    }

    /* initializari */
    clients = new client_entry[config->nr_contenders];
    num_connected_clients = 0;
    num_waiting_clients = 0;
    sem_init(&mutex_login,0,1);
    /* afisam codul concursului */
    printf("Codul concursului va fi: %d\n",config->code);
    /* citim portul pentru bind */
    printf("Portul serverului: ");
    scanf("%d",&port_commands);

    /* deschidem un socket in protocol TCP
     * */
    struct sockaddr_in server_commands;
    server_commands.sin_family = AF_INET;
    server_commands.sin_addr.s_addr = htonl(INADDR_ANY);
    server_commands.sin_port = htons((uint16_t)port_commands);
    if ((sock_connect = socket(AF_INET, SOCK_STREAM,0)) == -1)
        FAT_ERROR(socket);
    /* setam FLAG-ul REUSEADDR
     * */
    int opt_val = 1;
    if(setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR,&opt_val,sizeof(opt_val)) == -1)
        FAT_ERROR(setsockopt);
    /* facem bind la portul dorit
     * */
    if(bind(sock_connect,(struct sockaddr*)&server_commands,sizeof(struct sockaddr)) == -1)
        FAT_ERROR(bind);
    /* pornim listen pe socketul respectiv
     * */
    if(listen(sock_connect,config->nr_contenders) == -1)
        FAT_ERROR(listen);

    /* pregatim datele necesare primitivei select
     * */
    int nfds = sock_connect;
    fd_set read_fds,followed_fds;
    FD_ZERO(&followed_fds); /* initializam lista de descriptori */
    FD_SET(sock_connect,&followed_fds); /* adaugam socket-ul in lista */
    struct timeval select_time;
    select_time.tv_sec = 0;
    select_time.tv_usec = 100000;

    /* pornim timer-ul de conectare
     * */
    login_timer = new Timer(CLOCK_REALTIME);
    login_timer->start(config->nr_sec_wait);
    while( true )
    {
        /* pregatim apelul primitivei select
         * */
        select_time.tv_sec = 0;
        select_time.tv_usec = 100000;
        bcopy(&followed_fds,&read_fds, sizeof(fd_set));

        if(select(nfds+1,&read_fds, nullptr, nullptr,&select_time)<0)
            FAT_ERROR(select);

        // daca avem un client de acceptat
        if(FD_ISSET(sock_connect,&read_fds))
        {
            struct sockaddr_in new_client;
            size_t length = sizeof(struct sockaddr_in);
            bzero(&new_client,length);

            int sock_new_client;
            if((sock_new_client = accept(sock_connect,(struct sockaddr*)&new_client,(socklen_t*)&length)) == -1 )
                ERROR(accept);
            else{ // accept efectuat cu succes
                if( enable_keepalive(sock_new_client) == -1 )
                    ERROR(enable_keepalive);
                else{ // FLAG KEEPALIVE setat cu succes
                    if (nfds < sock_new_client)
                        nfds = sock_new_client;
                    FD_SET(sock_new_client, &followed_fds);
                }
            }
        }

        // trecem prin toti file descriptorii
        for(int fd=0; fd<=nfds; fd++){
            // daca fd e pregatit de citire si nu e socket-ul de conectare
            if(fd!= sock_connect && FD_ISSET(fd,&read_fds)) {
                // pregatim datele ce vor fi date thread-ului

                // clientul este unul dintre clientii online
                client_entry* my_client = nullptr;
                for (int i = 0; i < num_connected_clients; ++i) {
                    if(clients[i].socket == fd && !clients[i].line_closed){
                        my_client = &clients[i];
                    }
                }

                // clientul incearca sa se conecteze
                if(my_client == nullptr){
                    my_client = new client_entry;
                    my_client->socket = fd;
                }

                /* threadul main nu se va mai ocupa de urmarirea schimbului de mesaje
                 * cu acest client pana la terminarea tratarii comenzii de catre noul thread
                 * */
                FD_CLR(my_client->socket,&followed_fds);
                my_client->thread_blocked = true;

                /* pornim thread-ul ce va trata aceasta comanda
                 */
                pthread_t tid;
                pthread_create(&tid,nullptr,serve_client,my_client);
            }
        }

        // pentru toti participantii conectati
        for (int i = 0; i < num_connected_clients; ++i){
            // daca timpul alocat rezolvarii problemelor s-a scurs
            if( clients[i].timer.check() < 0 ){
                /* daca nu a trimis deja sursele, nu are o alta comanda in executie acum
                 * si nu a fost deja rugat sa trimita sursele
                 * */
                if( !clients[i].waiting_rez && !clients[i].thread_blocked && !clients[i].forced_to_submit){
                    /* il rog sa trimita sursele cu ajutorul trimiterii unei comenzi
                     */
                    clients[i].forced_to_submit = true;

                    /* threadul main nu se va mai ocupa de urmarirea schimbului de mesaje
                    * cu acest client pana la terminarea tratarii comenzii de catre noul thread
                    * */
                    if(FD_ISSET(clients[i].socket,&followed_fds))
                        FD_CLR(clients[i].socket,&followed_fds);
                    clients[i].thread_blocked = true;

                    /* pornim thread-ul ce va trata aceasta comanda
                     */
                    pthread_t tid;
                    pthread_create(&tid,nullptr,ans_req,&clients[i]);
                }
            }

            /* daca pentru un client ce este inca online si a carui mesaje nu sunt urmarite de threadul main
             * i s-a terminat thread-ul ce ii trata comanda precedenta
             * */
            if(!clients[i].line_closed && !clients[i].thread_blocked && !FD_ISSET(clients[i].socket,&followed_fds)){
                /* urmaresc iar posibila aparitia a noi comenzi de la client
                 * prin intermediul primitivei select
                 */
                FD_SET(clients[i].socket,&followed_fds);
            }
        }

        // daca timpul de login s-a scurs si toti clientii si-au trimis sursele
        if( !accepting_rez_req && login_timer->check()<0 && num_waiting_clients >= num_connected_clients ){
            // pregatesc fisierul cu clasamentul concursului
            sort_and_print(clients,num_connected_clients,"Rez.txt");
            // pot inchide serverul oricand vreau eu fara sa mai afectez rezultatele clientilor
            printf("Am primit si notat sursele tuturor clientilor.\n"
                   "Acestia pot cere rezultatele prin comanda \"rez\" incepand de acum.\n"
                   "Serverul poate fi acum inchis prin ctrl+C fara a afecta concursul.\n");
            // pot primi cereri de trimitere a rezultatelor
            accepting_rez_req = true;
        }
    }
}

static void* serve_client(void* data)
{
    auto my_client = (client_entry*)data;
    /* citim comanda de la client
     * */
    COMMAND command;
    data = &command;
    if( readmsg(my_client->socket,data) == -1 ){
        logout(my_client);
        return nullptr;
    }
    // setam thread-ul ca fiind detached
    pthread_detach(pthread_self());

    /* tratam comanda in functie de tipul acesteia
     * */
    switch(command)
    {
        case LOGOUT:{
            logout(my_client);
            if(my_client->name[0] == '\0') {
                delete my_client;
                my_client = nullptr;
            }
            return nullptr;
        }
        case LOGIN:{
            // semafor pt login:
            sem_wait(&mutex_login);
            // apelam functia login
            my_client = login(my_client);
            // anunt semaforul ca am teminat
            sem_post(&mutex_login);
            break;
        }
        case CONTEST_INFO_REQ:{
            if( sendCONTEST_INFO(my_client->socket,config->nr_sec_contest,config->nr_prob,config->files) == -1 ){
                logout(my_client);
                return nullptr;
            }
            if( sendALRIGHT(my_client->socket,"Informatii trimise cu succes!\n") == -1 ){
                logout(my_client);
                return nullptr;
            }
            break;
        }
        case CONTEST_BASIC_DATA_REQ:{
            if( sendCONTEST_BASIC_DATA(my_client->socket,config->nr_sec_contest,config->nr_prob,config->files) == -1 ){
                logout(my_client);
                return nullptr;
            }
            if( sendALRIGHT(my_client->socket,"Informatii trimise cu succes!\n") == -1 ){
                logout(my_client);
                return nullptr;
            }
            break;
        }
        case CONTEST_ANS:{
            if(my_client->waiting_rez){
                /* daca clientul a trimis deja un set de surse,
                 * restul seturilor trimise vor fi ignorate
                 * */
                if( sendREJECT(my_client->socket, "Ai trimis deja raspunsul.\n") == -1 ){
                    logout(my_client);
                    return nullptr;
                }
                if( receiveCONTEST_ANS(my_client->socket,config->nr_prob,config->files, "trash") == -1 ){
                    logout(my_client);
                    return nullptr;
                }
            }
            else {
                // salvam timpul in care clientul a rezolvat problemele
                int sec = my_client->timer.check();
                if( sec < 0 ) sec = 0;
                my_client->time = config->nr_sec_contest - sec;


                // primim sursele de la acesta
                char dir[100];
                dir[0] = '\0';
                strcat(dir, "rez/");
                strcat(dir, my_client->name);

                if( receiveCONTEST_ANS(my_client->socket, config->nr_prob, config->files, dir) == -1 ){
                    logout(my_client);
                    return nullptr;
                }

                // ii notam rezolvarea
                compute_grade(my_client,config);
                // clientul poate doar sa astepte rezultatele din acest moment
                my_client->waiting_rez = true;
                ++num_waiting_clients;

                if( sendALRIGHT(my_client->socket,"Surse trimise si notate cu succes!\n") == -1 ){
                    logout(my_client);
                    return nullptr;
                }
            }
            break;
        }
        case CONTEST_REZ_REQ:{
            if(accepting_rez_req){
                // daca rezultatele au fost deja calculate, le trimitem
                if( sendCONTEST_REZ(my_client->socket,"Rez.txt") == -1 ){
                    logout(my_client);
                    return nullptr;
                }

                if( sendALRIGHT(my_client->socket,"Rezultate trimise cu succes!\n") == -1 ){
                    logout(my_client);
                    return nullptr;
                }

            }
            else{
                if( sendREJECT(my_client->socket, "Rezultatele nu sunt accesibile inca.\n") == -1 ){
                    logout(my_client);
                    return nullptr;
                }
            }
            break;
        }
        case TIME_REQ:{
            int sec = my_client->timer.check();
            if( sec < 0 ) sec = 0;

            if( sendTIME(my_client->socket,sec) ){
                logout(my_client);
                return nullptr;
            }

            break;
        }
        default: {
            printf("\nAm primit o comanda ce nu ma priveste: %d\n",(int)command);
            break;
        }
    }

    // anuntam terminarea tratarii comenzii pentru acest client
    if(my_client != nullptr)
        my_client->thread_blocked = false;
    return nullptr;
}

static void* ans_req(void* data){
    auto my_client = (client_entry*)data;

    /* daca clientul e offline la terminarea concursului
     * si nu a trimis nicio sursa pana acum
     * il descalificam*/
    if(my_client->line_closed){
        my_client->waiting_rez = true;
        ++num_waiting_clients;
        my_client->comments = "DESCALIFICAT!";
    }
    else {
        // daca e online, il rugam sa trimita rezultatele
        if (sendCONTEST_ANS_REQ(my_client->socket) == -1) {
            logout(my_client);
            return nullptr;
        }
    }

    // anuntam terminarea tratarii comenzii pentru acest client
    my_client->thread_blocked = false;
    return nullptr;
}

client_entry* login(client_entry* my_client){
    // citim datele de login ale noului client
    char name[50],pass[25];
    int code;
    if( receiveLOGIN(my_client->socket,name,pass,code) == -1 ){
        logout(my_client);
        delete my_client;
        return nullptr;
    }

    // il cautam printre ceilati clienti
    bool existent = false;
    for (int i = 0; i < num_connected_clients && !existent; ++i) {
        if( strcmp(name,clients[i].name) == 0 &&
            strcmp(pass,clients[i].pass) == 0 &&
            code == config->code){
            // este un client mai vechi
            existent = true;
            if(clients[i].line_closed) {
                // nu este online deja => il actualizam ca fiind online la noul socket
                clients[i].socket = my_client->socket;
                clients[i].line_closed = false;
                delete my_client;
                my_client = &clients[i];
            }
            else{
                // este deja online => un cont nu poate fi conectat de mai multe ori simultan
                sendREJECT(my_client->socket,"Acest utilizator este deja autentificat!\n");
                logout(my_client);
                delete my_client;
                return nullptr;
            }
        }
    }
    // nu este un client mai vechi
    if(!existent) {
        // il adaugam daca mai putem
        if (login_timer->check() < 0) {
            // timpul de login s-a scurs
            sendREJECT(my_client->socket, "Timpul de autentificare s-a scurs!\n");
            logout(my_client);
            delete my_client;
            return nullptr;
        } else if (num_connected_clients == config->nr_contenders) {
            // numarul maxim de concurenti a fost deja atins
            sendREJECT(my_client->socket, "Numarul limita de clienti a fost deja atins!\n");
            logout(my_client);
            delete my_client;
            return nullptr;
        } else {
            // il putem adauga!
            strcpy(clients[num_connected_clients].name, name);
            strcpy(clients[num_connected_clients].pass, pass);
            // verificam corectitudinea datelor de login
            if (config->search_client(clients[num_connected_clients]) && code == config->code) {
                // date corecte => adaugam noul client in vectorul de clienti
                clients[num_connected_clients].timer = Timer(CLOCK_REALTIME);
                clients[num_connected_clients].socket = my_client->socket;
                clients[num_connected_clients].waiting_rez = false;
                clients[num_connected_clients].line_closed = false;
                clients[num_connected_clients].thread_blocked = true;
                clients[num_connected_clients].forced_to_submit = false;
                clients[num_connected_clients].timer.start(config->nr_sec_contest);

                delete my_client;
                my_client = &clients[num_connected_clients];

                num_connected_clients++;
            } else {
                // date incorecte => anuntam faptul ca autentificarea a esuat
                sendREJECT(my_client->socket, "Date de login gresite! Va rugam mai incercati o data:\n");
                logout(my_client);
                delete my_client;
                return nullptr;
            }
        }
    }

    // Anuntam clientul ca s-a logat cu succes
    if( sendACCEPT(my_client->socket) == -1 ){
        logout(my_client);
        delete my_client;
        return nullptr;
    }
    // ii trimitem informatiile despre concurs
    if(!existent) {
        // inclusiv fisierele problemelor ce trebuie rezolvate, daca acesta este un client nou
        if (sendCONTEST_INFO(my_client->socket, config->nr_sec_contest, config->nr_prob, config->files) == -1) {
            logout(my_client);
            delete my_client;
            return nullptr;
        }
    }
    else{
        if( sendCONTEST_BASIC_DATA(my_client->socket, config->nr_sec_contest, config->nr_prob, config->files) == -1 ){
            logout(my_client);
            delete my_client;
            return nullptr;
        }
    }
    if( sendALRIGHT(my_client->socket,"Autentificare terminata cu succes!\n") == -1 ){
        logout(my_client);
        delete my_client;
        return nullptr;
    }
    return my_client;
}

void logout(client_entry* my_client){
    // inchidem socket-ul clientului
    close(my_client->socket);
    printf("Clientul %s s-a deconectat. Am inchis socket-ul %d\n",my_client->name,my_client->socket);
    // actualizam datele clientului pentru a indica faptul ca acesta s-a deconectat
    my_client->line_closed = true;
    my_client->thread_blocked = false;
}

void sig_handler(int signo)
{
    printf("\nServer inchis!\n");
    close(sock_connect);
    // eliberam memoria alocata si inchidem programul
    sem_destroy(&mutex_login);
    delete login_timer;
    delete config;
    delete[] clients;
    exit(0);
}