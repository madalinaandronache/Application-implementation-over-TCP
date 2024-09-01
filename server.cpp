#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <regex>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

#include "utils.hpp"

// Map intre fiecare pereche TCP socket file descriptor si ID-ul clientului asociat
map<int, string> client_ids;

// Map intre fiecare pereche <TOPIC> si o lista de TCP file descriptor ai clientilor abonati
map<string, vector<int>> topic_subscriptions;

// Map intre fiecare file descriptor al clientului TCP si wildcard-ul asociat
map<int, vector<string>> wildcard_subscriptions;

void init_server(int argc, char *argv[], int &tcp_socket_fd, int &udp_socket_fd)
{
    DIE(argc != 2, "Incorect number of arguments!\nUsage: ./server <PORT_SERVER>\n");

    // Initializam socket-ul TCP si UDP pentru comunicarea client-server
    DIE((tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0, "TCP socket error\n");
    DIE((udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0, "UDP socket error\n");

    // Portul este reprezentat de argv[1]
    uint16_t port;
    DIE(sscanf(argv[1], "%hu", &port) != 1, "Given port is invalid\n");
    DIE(port < PORT_MIN_NUB || port > PORT_MAX_NUM, "Given port is invalid\n");

    // Initializam adresa serverului
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    // Setam portul serverului
    serv_addr.sin_port = htons(port);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int flag = 1;
    // Dezactivam algoritmul lui Nagle pentru socket-ul TCP
    DIE(setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Nagle error\n");

    // Permitem reutilizarea adreselor pentru ambele socket-uri
    DIE(setsockopt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0, "TCP socket reusable error\n");
    DIE(setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0, "UDP socket reusable error\n");

    // Legam socket-urile la adresa serverului
    DIE(bind(tcp_socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0, "TCP bind error\n");
    DIE(bind(udp_socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0, "UDP bind error\n");

    // Ascultam pe socket-ul pasiv de TCP
    DIE(listen(tcp_socket_fd, CLIENTS_MAX_NUM) < 0, "TCP listen error\n");
}

// daca id-ul a fost deja folosit, va intoarce true, altfel false
bool check_existing_id(const string &id)
{
    // daca al doilea element al perechii este egal cu id-ul
    for (auto &pair : client_ids)
    {
        if (pair.second == id)
        {
            // elementul a fost deja folosit
            return true;
        }
    }

    return false;
}

// Realizam conectarea unui nou client TCP, fara a duplica clientii care au acelasi ID
void new_tcp_client(int tcp_socket_fd, struct pollfd poll_fds[], int &poll_num)
{
    struct sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);

    // File descriptorul asociat noului client conectat
    int new_socket_fd;
    DIE((new_socket_fd = accept(tcp_socket_fd, (struct sockaddr *)&client_addr, &cli_len)) < 0, "Accept TCP client error\n");

    // Primul lucru primit este chiar ID-ul clientului
    char new_client_id[ID_MAX_LEN];
    memset(new_client_id, 0, ID_MAX_LEN);

    DIE(recv(new_socket_fd, new_client_id, ID_MAX_LEN, 0) < 0, "Receving from TCP client error\n");

    bool id_exists = check_existing_id(new_client_id);

    if (!id_exists)
    {
        // Adaugam noul file descriptor pentru a fi monitorizata comunicarea cu noul client
        poll_fds[poll_num].fd = new_socket_fd;
        poll_fds[poll_num].events = POLLIN;
        // Update-ul la numarul de conexiuni
        poll_num++;

        // mapam file descriptor-ul cu ID-ul clientului
        client_ids[new_socket_fd] = new_client_id;

        int flag = 1;
        // Dezactivam algoritmul lui Nagle pentru noul socket-ul TCP
        DIE(setsockopt(new_socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Nagle error\n");

        // Afisam mesajul corespunzator, conform enuntului
        printf("New client %s connected from %s:%d.\n", new_client_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
    else
    {
        // Afisam mesajul corespunzator, conform enuntului
        printf("Client %s already connected.\n", new_client_id);
        // Inchidem conexiunea
        close(new_socket_fd);
    }
}

void disconnected_client(int i, struct pollfd poll_fds[], int &poll_num)
{
    printf("Client %s disconnected.\n", client_ids[poll_fds[i].fd].c_str());

    // Stergem clientul din map si inchidem conexiunea
    client_ids.erase(poll_fds[i].fd);
    close(poll_fds[i].fd);

    // Stergem clientul care s-a deconectat din lista de file descriptori
    for (int j = i; j < poll_num - 1; j++)
    {
        poll_fds[j] = poll_fds[j + 1];
    }

    // Update la numarul de clientii
    poll_num--;
}

// Comanda primita este *exit*, deconectam toti socketii
void disconect_all(struct pollfd poll_fds[], int poll_num)
{
    for (int i = 1; i < poll_num; i++)
    {
        close(poll_fds[i].fd);
    }
}

void send_to_topic_subscriptions(const std::string &topic, const std::string &formatted_msg, std::set<int> &notified_clients)
{
    // Cautam topicul in map-ul de topicuri normale
    auto subscription_it = topic_subscriptions.find(topic);

    // Daca il gasim
    if (subscription_it != topic_subscriptions.end())
    {

        // Trimitem mesajul catre fiecare subscriber al topicului respectiv
        for (int client_socket_fd : subscription_it->second)
        {
            // Incercam sa inseram clientul in lista de clienti notificati
            if (notified_clients.insert(client_socket_fd).second)
            {
                // Daca a fost inserat cu succes, inseamna ca mesajul nu a fost inca trimis la el
                send(client_socket_fd, formatted_msg.c_str(), formatted_msg.length(), 0);
            }
        }
    }
}

bool matches_wildcard(const std::string &topic, const std::string &pattern)
{
    // Convertim sablonul wildcard într-un sablon regex
    std::regex star_replace(R"(\*)");
    std::regex plus_replace(R"(\+)");

    // Inlocuim '*' cu '.*' si '+' cu '[^/]+'
    std::string wildcard_pattern = std::regex_replace(pattern, star_replace, ".*");
    wildcard_pattern = std::regex_replace(wildcard_pattern, plus_replace, "[^/]+");

    // Creez sabonul final
    std::string regex_pattern = "^" + wildcard_pattern + "$";
    std::regex wildcard_regex(regex_pattern);

    // Verificam daca topic se potrivește cu sablonul
    return std::regex_match(topic, wildcard_regex);
}

void send_to_wildcard_subscriptions(const std::string &topic, const std::string &formatted_msg, std::set<int> &notified_clients) {
    // Iteram prin toate subscriptiile pe care le avem
    for (const auto &[sockfd, patterns] : wildcard_subscriptions) {
        // Daca clientul TCP nu a fost deja notificat

        if (notified_clients.find(sockfd) == notified_clients.end()) {
            // Verificam pentru fiecare topic la care este abonat

            for (const auto &pattern : patterns) {

                // Daca se potriveste cu topicul curent de pe care am primit mesajul UDP
                if (matches_wildcard(topic, pattern)) {

                    // Trimitem mesajul catre client
                    send(sockfd, formatted_msg.c_str(), formatted_msg.length(), 0);

                    // Adaugam clientul in lista de clienti notificati
                    notified_clients.insert(sockfd);
                    break;
                }
            }
        }
    }
}

void send_to_subscribers(const udp_msg_t &msg, const std::string &ip_address, uint16_t port)
{
    // Formatam mesajul primit de la clientul UDP pentru a fi trimis mai departe
    std::string formatted_msg = process_udp_message(msg, ip_address, port);

    // Set cu file descriptorii clientilor ce au fost deja notificati
    set<int> notified_clients;

    // Incercam sa trimitem catre utilizatorii abonati la topicurile normale
    send_to_topic_subscriptions(msg.topic, formatted_msg, notified_clients);

    // Incercam sa trimitem catre utilizatorii abonati la topicurile cu wildcard
    send_to_wildcard_subscriptions(msg.topic, formatted_msg, notified_clients);
}

void unsubscribe(int sockfd, const std::string &topic)
{
    // Lista tuturor clientilor abonati la topicul respectiv
    auto &client_list = topic_subscriptions[topic];
    // Stergem aparitia clientului cu file descriptorul sockfd
    client_list.erase(std::remove(client_list.begin(), client_list.end(), sockfd), client_list.end());

    // Lista wildcard-urilor la care clientul cu file descriptorul sockfd este abonat
    auto &patterns = wildcard_subscriptions[sockfd];
    patterns.erase(std::remove(patterns.begin(), patterns.end(), topic), patterns.end());

    // Trimitem mesajul inapoi catre client
    DIE(send(sockfd, ("Unsubscribed from topic " + topic + "\n").c_str(), topic.length() + 24, 0) < 0, "Unsubscription error\n");
}

void received_udp_message(int udp_fd)
{
    struct sockaddr_in udp_addr;
    socklen_t udp_len = sizeof(udp_addr);

    // Primim un mesaj de tip UDP
    udp_msg_t msg;
    DIE(recvfrom(udp_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&udp_addr, &udp_len) < 0, "Receiving UDP message error\n");

    // Aflam adresa IP si portul clientului UDP
    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(udp_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
    uint16_t port = ntohs(udp_addr.sin_port);

    // Trimitem mesajul primit la abonatii TCP
    send_to_subscribers(msg, ip_address, port);
}

void subscribe(int sockfd, const std::string &topic)
{
    // Daca topicul contine unul dintre caracterele "*" sau "+"
    if (topic.find('*') != std::string::npos || topic.find('+') != std::string::npos)
    {
        // Inseamna ca avem un wildcard
        wildcard_subscriptions[sockfd].push_back(topic);
    }
    else
    {
        // Altfel, topicul este unul normal
        topic_subscriptions[topic].push_back(sockfd);
    }

    // Trimitem mesajul inapoi catre client
    DIE(send(sockfd, ("Subscribed to topic " + topic + "\n").c_str(), topic.length() + 22, 0) < 0, "Subscription error\n");
}

// Procesam comanda primita de la un client TCP
void process_client_command(const int sockfd, char cmd[])
{
    auto [command, topic] = get_topic_and_command(cmd);

    if (!command.empty() && !topic.empty())
    {
        if (command == "subscribe")
        {
            // Daca comanda este subscribe
            subscribe(sockfd, topic);
        }
        else if (command == "unsubscribe")
        {
            // Daca comanda este unsubscribe
            unsubscribe(sockfd, topic);
        }
        else
        {
            // Avem o eroare
            fprintf(stderr, "Unknown command!\n");
        }
    }
    else
    {
        // Avem o eroare
        fprintf(stderr, "Unknown command!\n");
    }
}

void run_server(int tcp_socket_fd, int udp_socket_fd)
{
    // Configuram monitorizarea pentru receptia de mesaje
    struct pollfd poll_fds[CLIENTS_MAX_NUM];
    int poll_num = 3;

    poll_fds[0].fd = STDIN_FILENO;
    // Activam flag-ul care ne arata ca sunt date disponibile pentru a fi citite
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = udp_socket_fd;
    poll_fds[1].events = POLLIN;

    poll_fds[2].fd = tcp_socket_fd;
    poll_fds[2].events = POLLIN;

    // Buffer folosit pentru mesaje
    char buf[MSG_MAX_SIZE + 1];
    memset(buf, 0, MSG_MAX_SIZE + 1);

    while (true)
    {
        DIE(poll(poll_fds, poll_num, POLL_TIMEOUT) < 0, "Poll fail\n");

        // Daca avem date de intarare de la STDIN
        if (poll_fds[0].revents & POLLIN)
        {
            // Citim datele de la STDIN
            DIE(fgets(buf, sizeof(buf), stdin) == NULL, "Input error\n");

            // Daca comanda este de "exit", inchidem atat serverul, cat si clientii
            if (strncmp(buf, "exit", 4) == 0)
            {
                disconect_all(poll_fds, poll_num);
                return;
            }
            else
            {
                fprintf(stderr, "Unknown command!\n");
            }
        }
        else if (poll_fds[1].revents & POLLIN)
        {
            // Daca avem date de intrare primite de la socket-ul UDP
            received_udp_message(poll_fds[1].fd);
        }
        else if (poll_fds[2].revents & POLLIN)
        {
            // Daca avem date de intrare primite de la socket-ul pasiv
            new_tcp_client(poll_fds[2].fd, poll_fds, poll_num);

            continue;
        }

        // Daca avem date de intrare primite de la clientii TCP
        for (int i = 3; i < poll_num; i++)
        {
            if (poll_fds[i].revents & POLLIN)
            {
                int rc = recv(poll_fds[i].fd, buf, sizeof(buf), 0);
                DIE(rc < 0, "Receiving data from TCP client error\n");

                if (rc != 0)
                {
                    process_client_command(poll_fds[i].fd, buf);
                }
                else
                {
                    // Socket-ul a fost inchis, deci clientul s-a deconectat
                    disconnected_client(i, poll_fds, poll_num);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    // Dezactivam buffering-ul la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Variabile pentru udp & tcp socket file descriptors
    int tcp_socket_fd, udp_socket_fd;

    // Initializam serverul
    init_server(argc, argv, tcp_socket_fd, udp_socket_fd);
    // Rularea server-ului
    run_server(tcp_socket_fd, udp_socket_fd);

    close(tcp_socket_fd);
    close(udp_socket_fd);

    return 0;
}
