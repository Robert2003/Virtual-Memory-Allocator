**Nume: Damian Mihai-Robert**<br>
**Grupă: 312CA**

## Virtual Memory Allocator
<br>

# Descriere:

* Programul simuleaza o memorie virtuala folosind conceptul de lista in<br>
  lista(lista dublu inlantuita)
  * Memoria virtuala este simulata de o arena, care contine o lista de blockuri
  * Fiecare block contine, de asemenea, o lista de miniblockuri
  * Daca doua blockuri ajung sa aiba capete comune, atunci acestea se unesc<br>
    si devin un singur block
  * Daca se sterge un miniblock care nu este capat intr-un block, atunci<br>
    blockul se imparte in doua
  * Daca se sterge singurul miniblock din block, atunci se sterge si blockul
  * Continutul nodurilor din liste, sunt de tip (void *), asa ca acestea pot<br>
    sa fie atat alte liste, cat si miniblockuri in sine
  * peratia MPROTECT schimba permisiunile unui miniblock
  * Se pot realiza operatii de read/write in miniblockurile din acelasi<br>
    block, dar daca nu exista permisiuni in acest sens, operatiile nu<br>
    se pot realiza
* Explicatii suplimentare asupra codului:
  * Daca se adauga un block care are margine comuna cu blockul din stanga,<br>
    atunci nu se va crea alt block, ci miniblockul se va adauga la finalul<br>
    blockului din stanga. Daca dupa adaugare, are margine comuna si cu cel<br>
    din dreapta, atunci lista blockului din dreapta se va pune la finalul<br>
    listei blockului din stanga si se va sterge blockul din dreapta
  * Daca se adauga un block care are margine comuna cu blockul din<br>
    dreapta, atunci miniblockul se va adauga la inceputul blockului din dreapta
   * Daca nu are margini comune cu niciun block, atunci se va crea altul

<br>

# Comentarii asupra temei:

* Crezi că ai fi putut realiza o implementare mai bună?
  * Mereu exista loc de imbunatatiri
* Ce ai invățat din realizarea acestei teme?
  * Am invatat cum se foloseste lista in lista si am aprofundat (void *)
  * Am descoperit multe alte moduri in care poti lua seg fault
* Alte comentarii
  * Cerinta temei este greu de inteles, cu multe lucruri interpretabile

<br>

# Comenzi posibile:

## 1. Operatia ALLOC_ARENA
#### Utilizare:
```
ALLOC_ARENA <dimensiune_arena>
```
#### Descriere:
```
  Se alocă un buffer contiguu de dimensiune ce va fi folosit pe post de kernel
buffer sau arenă
```
<br>

## 2. Operatia DEALLOC_ARENA
#### Utilizare:
```
DEALLOC_ARENA
```
#### Descriere:
```
  Se eliberează arena alocată la începutul programului.
```
<br>

## 3. Operatia ALLOC_BLOCK
#### Utilizare:
```
ALLOC_BLOCK <adresă_din_arenă> <dimensiune_block>
```
#### Descriere:
```
  Se marchează ca fiind rezervată o zonă ce începe la adresa <adresă_din_arenă>
  în kernel buffer cu dimensiunea de <dimensiune_block>
```
<br>

## 4. Operatia FREE_BLOCK
#### Utilizare:
```
FREE_BLOCK <adresă_din_arenă>
```
#### Descriere:
```
  Se eliberează un miniblock.
  Dacă se eliberează unicul miniblock din cadrul unui block, atunci block-ul
este la rândul său eliberat.
  Dacă se eliberează un miniblock din mijlocul block-ului, atunci acesta va fi
împărțit în două block-uri distincte.
```
<br>

## 5. Operatia READ
#### Utilizare:
```
READ <adresă_din_arenă> <dimensiune>
```
#### Descriere:
```
Se afișează <dimensiune> bytes începând cu adresa <adresă_din_arenă>, iar la
final \n.
```
<br>

## 6. Operatia WRITE
#### Utilizare:
```
WRITE <adresă_din_arenă> <dimensiune> <date>
```
#### Descriere:
```
Se scriu <dimensiune> bytes din <date> la adresa <adresă_din_arenă>.
```
<br>

## 7. Operatia PMAP
#### Utilizare:
```
PMAP
```
#### Descriere:
```
Se afișează informații despre block-urile și miniblock-urile existente.
```
<br>

## 8. Operatia MPROTECT
#### Utilizare:
```
MPROTECT <adresă_din_arenă> <noua_permisiune>
```
#### Descriere:
```
Schimbă permisiunile zonei care începe la <adresă_din_arenă> de dimensiune
<dimensiune> din default-ul RW- în <noua_permisiune>.
```

#### Exemplu de utilizare:
```
MPROTECT 256 PROT_READ | PROT_WRITE | PROT_EXEC
MPROTECT 256 PROT_NONE
```

<br>
