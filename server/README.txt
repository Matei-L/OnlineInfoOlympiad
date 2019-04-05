Fisierele de configurare sunt: server_config.json problems.json si contenders.json

server_config.json trebuie sa contina:
code: int / "random"
nr_contenders: int
nr_prob: int
nr_sec_contest: int
nr_sec_wait: int
prob_choosing_type: "select" / "random"
problems: o lista de probleme in formatul listei din problems.json (poate lipsi daca
        prob_choosing_type = "random")

contenders.json trebuie sa contina:
list: o lista de conturi, fiecare cont fiind format din
    name: un sir de 50 de caractere sau numere sau '_' dar fara spatii
    pass: un sir de 25 de caractere
    
problems.json trebuie sa contina:
list: o lista de probleme, fiecare problema fiind formata din
    path : path-ul unde se afla fisierele problemei
    name : numele fisierului .cpp ce contine problema
    in : numele fisierului cu datele de intrare exemplu
    out : numele fisierului cu datele de iesire exemplu
    test base : baza numelui fisierelor de test
    number tests : numarul testelors
    test in : in . ce se termina fisierele de test de intrare
    test out : in . ce se termina fisierele de test de iesire
    time : un float reprezentand timpul maxim de executie a unui test
