 /*Floyd-Warshall/Roy-Floyd
  *Se da un graf orientat cu N noduri, memorat prin matricea ponderilor.
  *
  *Cerinta
  *Sa se determine pentru orice pereche de noduri x si y lungimea minima a drumului de la nodul x la nodul y si sa se afiseze matricea drumurilor minime. Prin lungimea unui drum intelegem suma costurilor arcelor care-l alcatuiesc.
  *
  *Date de intrare
  *Fisierul de intrare royfloyd.in contine pe prima linie N, numarul de noduri al grafului, iar urmatoarele N linii contin cate N valori reprezentand matricea ponderilor (al j-lea numar de pe linia i+1 reprezinta costul muchiei de la i la j din graf, sau 0, in cazul in care intre cele doua noduri nu exista muchie).
  *
  *Date de iesire
  *In fisierul de iesire royfloyd.out se vor afisa N linii a cate N valori, reprezentand matricea drumurilor minime.
  *
  *Restrictii
  *1 ≤ N ≤ 100
  *Numerele din fisierul de intrare nu vor depasi valoarea 1 000.
  *Daca nu exista muchie intre o pereche de noduri x si y, distanta de la nodul x la nodul y din fisierul de intrare va fi 0.
  *Daca dupa aplicarea algoritmului nu se gaseste un drum intre o pereche de noduri x si y, se va considera ca distanta intre cele doua noduri este 0.
  *Nu exista drum intre un nod la el insusi ( ai,i este 0 pentru orice i cuprins intre 1 si N).
  * 
  * Timp execuţie pe test	0.05 sec
*/
