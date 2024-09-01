# Protocoale de comunicatii - Tema 2

Student: Andronache Madalina-Georgiana

Grupa: 322CC

Urmatorul fisier contine informatii despre rezolvarea cerintelor propuse in 
tema 2 de la PCom: Aplicatie client-server TCP si UDP pentru gestionarea 
mesajelor.

## Cuprins:
1. Subscriber (Clientii TCP)
2. Server
3. Provocari
4. Referinte


Implementarea functionalitatilor client-server a fost realizata in 2 fisiere 
principale: server.cpp si subscriber.cpp. In plus, avem fisierul secundar utils.cpp 
siam header-ul utils.hpp care contine definirea functiilor:

```
// Functie care transforma un mesaj de tip UDP intr-un string formatat corespunzator
// pentru a fi trimis la clientii TCP
std::string process_udp_message(const udp_msg_t &msg, const std::string &ip_address, uint16_t port);

// Extragem comanda primita de la clientul TCP sub forma <Comanda> <Topic> (intr-un sir
// de caractere) si returneaza perechea formata din <Comanda, Topic>
std::pair<std::string, std::string> get_topic_and_command(char cmd[]);
```

a constantelor utilizate in cele 2 fisiere si a structurii:

```
// Structura care retine un mesaj UDP, care respecta formatul definit in enunt 
struct udp_msg_t {
  char topic[50];
  uint8_t type;
  char content[1500];
}; 
```

Cea mai mare provocare intalnita a fost rezolvarea corecta si cat mai eficienta
a problemei propuse intr-un timp cat mai scurt. Aceasta tema a fost rezolvata
pe parcursul a 4 zile: in total am lucrat la aceasta tema aproximativ 30 h. 
Punctajul obtinut la testarea locala este de 100/100 pct. 

# 1. Subscriber (Clientii TCP)

La inceputul programului este dezactivat buffering-ul la afisare folosind:
`setvbuf(stdout, NULL, _IONBF, BUFSIZ)`.

## 1.1. Pornirea unui subscriber

Pornirea unui subscriber (client TCP) se face cu urmatoarea comanda:
`./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>`, unde:
* `<ID>` reprezinta ID-ul clientului (un sir de maxim 10 caractere ASCII)
* `<IP_SERVER>` reprezinta adresa IPv4 a serverului reprezentata folosind 
notatia dotted-decimal (exemplu 1.2.3.4)
* `<PORT_SERVER>` reprezinta portul pe care serverul asteapta conexiuni.

Functia init_subscriber se ocupa de aceasta initializare conform enuntului.
Identificam urmatoarele probleme care pot aparea pana la acest punct:
* La pornirea clientului, utilizatorul introduce mai multe/mai putine argumente
in linia de comanda, caz in care se va afisa la STDERR mesajul 
**Incorect number of arguments! Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>**.
* Ne dorim conectarea clientului la server printr-un socket TCP, daca aceasta 
conecatare nu reuseste, se va afisa la STDERR mesajul **TCP socket error**.
* Daca numarul primit pentru port nu este cuprins in intervalul [1, 65535], se 
va afisa mesajul **Given port is invalid**.
* Daca adresa serverului nu este corect introdusa se va afisa mesajul 
**Inet error, incorrect IP address**.

In plus, dezactivam algoritmul Nagle, realizam conectarea socket-ului la 
server si transmitem id-ul clientului catre server. In cazul in care se va 
genera eroare  pentru una dintre operatiile de mai sus, afisam mesajele
**Nagle error**, **Connect socket error**, respectiv
**Client information send error**.

## 1.2. Comenzi acceptate

Clientii TCP pot primi de la tastatura una dintre urmatoarele comenzi:
* a. `subscribe <TOPIC>` anunta serverul ca un client doreste sa se aboneze la 
mesaje pe topic-ul dat
* b. `unsubscribe <TOPIC>` anunta serverul ca un client doreste sa se 
dezaboneze de la topic-ul dat
* c. `exit` comanda va fi folosita pentru deconectarea clientului de la server 
si inchiderea sa.

Functia run_subscriber se ocupa de aceste cazuri, astfel:
* daca primim date de intrare de la STDIN, daca comanda data incepe cu 
"subscribe" sau "unsubscribe", trimitem mesajul la server, pentru a realiza 
abonarea / dezabonarea, daca comanda este "exit", atunci inchidem socket-ul 
TCP si programul se opreste, altfel, daca utilizatorul introduce orice alta 
comanda, se va afisa mesajul **Unknown command!**.
* daca primim date de la server, prin socket, verificam, daca serverul a fost 
inchis, inchidem si subscriberii conectati prin socket la el, altfel, daca 
mesajul primit este corect (nu intampinam o eroare), atunci afisam mesajul 
primit.

Erorile care pot fi intampinate sunt urmatoarele:
* **Poll fail** - eroare de la monitorizarea pentru STDIN si socket file
descriptor
* **Input error** - eroare de la citirea datelor de la STDIN
* **Client subscription error** - eroare la trimiterea datelor de abonare
catre server.

# 2. Server

La inceputul programului este dezactivat buffering-ul la afisare folosind:
`setvbuf(stdout, NULL, _IONBF, BUFSIZ)`.

## 2.1. Pornirea unui server

Serverul va avea rolul de broker in platforma de gestionare a mesajelor. Acesta
va deschide 2 socketi (unul TCP si unul UDP) pe un port primit ca parametru si
va astepta conexiuni/datagrame pe toate adresele IP disponibile local.

Pornirea unui subscriber (client TCP) se face cu urmatoarea comanda:
`./server <PORT_SERVER>`, unde:
* `<PORT_SERVER>` reprezinta portul pe care serverul asteapta conexiuni.

Functia init_subscriber se ocupa de aceasta initializare conform enuntului.
Identificam urmatoarele probleme care pot aparea pana la acest punct:
* La pornirea serverului, utilizatorul introduce mai multe/mai putine argumente
in linia de comanda, caz in care se va afisa la STDERR mesajul 
**Incorect number of arguments! Usage: ./server <PORT_SERVER>**.
* Daca numarul primit pentru port nu este cuprins in intervalul [1, 65535], se 
va afisa mesajul **Given port is invalid**.
* Initializam cei 2 sockets (unul TCP si unul UDP), daca acest pas genereaza 
eroare se va afisa **TCP socket error**, respectiv **UDP socket error**.
* Dezactivam algoritmul Nagle, iar in caz de eroare afisam **Nagle error**.
* Permitem reutilizarea adreselor pentru ambele socket-uri, in caz de eroare
afisam **TCP socket reusable error**, respectiv **UDP socket reusable error**.
* Si legam (bind) socket-urile la adresa serverului, in caz de eroare afisand
**TCP bind error**, respectiv **UDP bind error**.

## 2.2. Functionarea server-ului

Functia principala care se ocupa de functionarea serverului este run_server.
Initial, la fel cum este aratat si la initializare, este de interes 
monitorizarea a 4 "cai/modalitati" prin care serverul poate primi mesaje: 

### a. STDIN - mesaje primite ca input de la utilizator

Conform enuntului, singura comanda pe care serverul o poate primi este "exit".
La primirea acestei comenzii se deconecteaza atat serverul, cat si toti clientii
conectati (folosind functia `disconect_all`). Daca utilizatorul introduce orice 
alta comanda, se va afisa mesajul **Unknown command!**.

### b. Socket UDP - pentru a intercepta mesaje primite de tip UDP

Se primeste mesajul UDP si se proceseaza pentru a fi transmis către clientii 
TCP abonati la topicul respectiv (funcția `received_udp_message`).

- `received_udp_message` - parseaza un mesaj de tip `udp_msg_t` si trimite
mesajul mai departe folosind functia `send_to_subscribers`.
    - `send_to_subscribers`- prima oara formatam mesajul primit de la clientul 
UDP, astfel incat sa poata fi trimis mai departe. In plus, deorece ne dorim ca 
un subscriber sa primeasca o singura data mesajul fie ca este abonat la un wildcard
fie la un topic normal, initializam un map care sa contina file descriptorii 
clientilor deja notificati. 
        - `send_to_topic_subscriptions` - incercam sa trimitem mesajul catre toti
utilizatorii care sunt abonati la topicul normal, avand in vedere ca mesajul sa fie
trimis o singura data.
        - `send_to_wildcard_subscriptions` - incercam sa trimitem mesajul catre
toti utilizatorii  care sunt abonati la topicuri wildcard si care nu au fost deja
notificati.
            - `matches_wildcard` - functie booleana folosita pentru a verifica 
potrivirea dintre un wildcard si un topic (folosim regex, vezi referinte).

### c. Socket pasiv TCP - pentru a accepta conexiuni noi de tip TCP cu clientii.
La care se adauga ulterior, monitorizarea fiecarui client TCP (subscriber) 
pentru a intercepta mesajele trimise de catre acestia.

Se accepta conexiunea de la un nou client si se adaugă socket-ul în lista 
de monitorizare pentru comunicare ulterioara (funcția `new_tcp_client`).

- `new_tcp_client` - initial, acceptam conexiunea deschisa prin socketul TCP,
in caz de eroare afisand **"Accept TCP client error**. Si setam id-ul noului
client cu mesajul primit de la client. 
    - `check_existing_id` - verificam daca ID-ul primit este deja folosit de 
un client, sau este liber, functia este de tip bool si intoarce `true` daca
ID-ul este deja folosit, `false` in caz contrar.
    - Daca un client TCP incearca sa se conecteze la server cu acelasi ID ca 
al altui client deja conectat, serverul va afisa mesajul: 
`Client <ID_CLIENT> already connected.` si va inchide noua conexiune acceptata
(clientul). 
    - Daca ID-ul este liber, adaugam noul file descriptor in lista pentru a fi 
monitorizata comunicarea cu noul client, dezactivam algoritmul Nagle si mapam
file descriptor-ul conexiunii TCP cu ID-ul clientului. In caz de eroare, afisam
**Nagle error**. In plus, serverul va afisa mesajul 
`New client <ID_CLIENT> connected from IP:PORT.`.


### d. Se verifica daca primim date de la oricare client TCP 

Daca datele citite reprezintă o comanda valida (abonare/dezabonare), 
aceasta se gestioneaza cu functia `process_client_command`. Altfel, 
daca conexiunea a fost inchisa de client, se elimina socket-ul clientului 
din lista de monitorizare si se afiseaza mesajul de deconectare conform
enuntului (functia `disconnected_client`).

- `process_client_command` - initial, folosim functia `get_topic_and_command`
pentru a determina perechea <comanda, topic>. Daca comanda este diferita de 
"subscribe"/"unsubscribe", se va afisa un mesaj de eorare `Unknown command!`.
    - `subscribe`- daca topicul contine unul dintre caracterele '*' sau '+',
inseamna ca avem un wildcard, caz in care adaugam in map-ul 
**wildcard_subscriptions** aceasta noua conectare. Altfel, insemna ca topicul 
este unul normal, adaugam in map-ul **topic_subscriptions**. Trimitem mesajul 
de conectare catre clientul TCP (subscriber) si in caz de eroare afisam mesajul
**Subscription error**.
    - `unsubscribe` - stergem topicul atat din map-ul **topic_subscriptions**, 
cat si din cel **wildcard_subscriptions**. Trimitem mesajul 
de dezabonare catre clientul TCP (subscriber) si in caz de eroare afisam mesajul
**Unsubscription error**.


# Provocari

* Daca foloseam strtok pentru a afla din mesajul primit de la clientul TCP
comanda si topicul primeam timeout in functia process_client_command.

* Am descoperit ca mesajul trimis inapoi initial catre client 
<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ> nu 
respecta intocmai structura, aveam space-uri intre IP si PORT si 
totusi checker-ul nu verifica acest lucru.

* Un DIE pus aiurea, m-a costat 2h de debugging si o panica de zile mari, 
crezand ca problema este de la faptul ca laptopul meu isi daduse update 
la BIOS =) . 

# Referinte

Regex for wildcards:
* https://stackoverflow.com/questions/37648340/how-to-match-string-with-wildcard-using-c11-regex
* https://en.cppreference.com/w/cpp/regex 

Client & Server:
* https://gitlab.cs.pub.ro/pcom/pcom-laboratoare-public/-/tree/master/lab7
