//
// Created by matei on 08.12.2018.
//

#include "Config.h"

Config::Config() {
    // deschidem fisierele de configurare si le citim
    ifstream f("server_config.json");
    f>>config;
    f.close();

    ifstream p("problems.json");
    p>>problems;
    p.close();

    ifstream c("contenders.json");
    c>>contenders;
    c.close();

    // cream un cod random pentru concurs daca ne este cerut
    random_device rd;
    mt19937 mt(rd());
    if(config["code"] == "random"){
        uniform_int_distribution<> dist(1000,9999);
        config["code"] = dist(mt);
    }

    // salvam informatiile despre concurs in variabilele publice
    code = config["code"];
    nr_prob = config["nr_prob"];
    nr_contenders = config["nr_contenders"];
    nr_sec_contest = config["nr_sec_contest"];
    nr_sec_wait = config["nr_sec_wait"];
    files = nullptr;

    // ne este cerut sa alegem random problemele?
    if(config["prob_choosing_type"] == "random"){ // DA
        int n = problems["list"].size(),n_rands = nr_prob;
        // verificam daca avem destule probleme disponibile
        if(n_rands>n){
            // daca nu, modificam numarul de probleme la numarul de probleme disponibile
            n_rands = n;
            nr_prob = n;
            config["nr_prob"] = n;
        }

        // alegem n_rands probleme distincte la intamplare
        int *rands = new int[n_rands];
        bool *picked = new bool[n];
        for(int i = 0; i < n; ++i) picked[i] = false;

        for (int i = 0; i < n_rands; ++i) {
            uniform_int_distribution<> dist(0,n-i-1);
            int val = dist(mt);
            int j = -1;
            while (val>=0){
                ++j;
                if(!picked[j]){
                    --val;
                }
            }
            picked[j] = true;
            rands[i] = j;
        }

        // le copiem in fisierul de configurare al serverului
        for (int k = 0; k < n_rands; ++k) {
            config["problems"][k] = problems["list"][rands[k]];
        }

        config["prob_choosing_type"] = "select";
        delete[] rands;
        delete[] picked;
    }
    else{ // NU
        // verificam daca avem destule probleme disponibile
        if(config["nr_prob"] > config["problems"].size()){
            // daca nu, modificam numarul de probleme la numarul de probleme disponibile
            config["nr_prob"] = config["problems"].size();
            nr_prob = config["nr_prob"];
        }
    }

    /* cream stringul files ce va fi trimis clientilor
     * acesta contine numele fisierelor ce vor fi timise
     * intre server si clienti separate prin cate un spatiu
     * */
    string files_construct;
    for (int i = 0; i < nr_prob; ++i) {
        files_construct += config["problems"][i]["path"];
        files_construct += config["problems"][i]["name"];
        files_construct += " ";
    }
    for (int i = 0; i < nr_prob; ++i) {
        files_construct += config["problems"][i]["path"];
        files_construct += config["problems"][i]["in"];
        files_construct += " ";

        files_construct += config["problems"][i]["path"];
        files_construct += config["problems"][i]["out"];
        files_construct += " ";
    }
    /* memoria alocata pentru acest string trebuie eliberata prin free
     */
    files = (char *) malloc(strlen(files_construct.c_str()) + 1);
    bcopy(files_construct.c_str(), files, strlen(files_construct.c_str()) + 1);
    /* punem datele despre acest concurs inapoi in server_confi.json
     */
    ofstream o("server_config.json");
    o<<config.dump(4);
    o.close();
}


bool Config::search_client(client_entry client) {
    /* cautam o potrivire a clientului dupa nume si prenume
     */
    string name,pass;
    for(int i = 0; i < contenders["list"].size(); ++i){
        name = contenders["list"][i]["name"];
        pass = contenders["list"][i]["pass"];
        if( strcmp(client.name,name.c_str()) == 0 &&
            strcmp(client.pass,pass.c_str()) == 0 ){
            return true;
        }
    }
    return false;
}

Config::~Config() {
    // eliberam memoria alocata dinamic
    free(files);
}

int Config::get_num_tests(int prob) {
    return config["problems"][prob]["number tests"];
}

string Config::get_test(int prob, int index) {
    char i[11];
    sprintf(i,"%d",index);
    string rez = config["problems"][prob]["path"];
    rez+=config["problems"][prob]["test base"];
    rez+=i;
    rez+=config["problems"][prob]["test in"];
    return rez;
}

string Config::get_test_rez(int prob, int index) {
    char i[11];
    sprintf(i,"%d",index);
    string rez = config["problems"][prob]["path"];
    rez+=config["problems"][prob]["test base"];
    rez+=i;
    rez+=config["problems"][prob]["test out"];
    return rez;
}

string Config::get_in(int prob) {
    return config["problems"][prob]["in"];
}

string Config::get_out(int prob) {
    return config["problems"][prob]["out"];
}

string Config::get_path(int prob) {
    return config["problems"][prob]["path"];
}

string Config::get_name(int prob) {
    return config["problems"][prob]["name"];
}

double Config::get_time(int prob) {
    return config["problems"][prob]["time"];
}
