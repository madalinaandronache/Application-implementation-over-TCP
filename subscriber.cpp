#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

using namespace std;

#include "utils.hpp"

void init_subscriber(int argc, char *argv[], int &udp_socket_fd, char &id)
{
    DIE(argc != 4, "Incorect number of arguments!\nUsage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");

    // Id-ul clientului este reprezentat de argv[1]
    memcpy(&id, argv[1], ID_MAX_LEN);

    // Initializam socket-ul TCP pentru comunicarea client-server
    DIE((udp_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0, "TCP socket error\n");

    // Portul este reprezentat de argv[3]
    uint16_t port;
    DIE(sscanf(argv[3], "%hu", &port) != 1, "Given port is invalid\n");
    DIE(port < PORT_MIN_NUB || port > PORT_MAX_NUM, "Given port is invalid\n");

    // Initializam adresa serverului
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    // Setam portul serverului
    serv_addr.sin_port = htons(port);
    serv_addr.sin_family = AF_INET;
    // Setam adresa serverului, reprezentata de argv[2]
    DIE(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr) <= 0, "Inet error, incorrect IP address\n");

    // Dezactivam algoritmul lui Nagle pentru socket-ul TCP
    int flag = 1;
    DIE(setsockopt(udp_socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Nagle error\n");

    // Conectam socket-ul la server
    DIE(connect(udp_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0, "Connect socket error\n");

    // Trimitem id-ul clientului ca prima transmisie
    DIE(send(udp_socket_fd, &id, ID_MAX_LEN, 0) < 0, "Client information send error\n");
}

void run_subscriber(int udp_socket_fd, char *id)
{
    // Configuram monitorizarea pentru STDIN si socketul TCP
    struct pollfd poll_fds[FD_MAX_NUM];
    poll_fds[0].fd = STDIN_FILENO;
    // Activam flag-ul care ne arata ca sunt date disponibile pentru a fi citite
    poll_fds[0].events = POLLIN;

    // Buffer folosit pentru mesaje
    char buf[MSG_MAX_SIZE + 1];
    memset(buf, 0, MSG_MAX_SIZE + 1);

    // Socketul cu file descriptorul egal cu argumentul primit
    poll_fds[1].fd = udp_socket_fd;
    poll_fds[1].events = POLLIN;

    while (true)
    {
        DIE(poll(poll_fds, FD_MAX_NUM, POLL_TIMEOUT) < 0, "Poll fail\n");

        // Daca avem date de intarare de la STDIN
        if (poll_fds[0].revents & POLLIN)
        {
            // Citim datele de la STDIN
            DIE(fgets(buf, sizeof(buf), stdin) == NULL, "Input error\n");

            // Daca comanda este de "subscribe"/"unsubscribe" trimitem datele prin socket
            if (strncmp(buf, "subscribe", 9) == 0 || strncmp(buf, "unsubscribe", 11) == 0)
            {
                DIE(send(udp_socket_fd, buf, strlen(buf), 0) < 0, "Client subscription error\n");
            }
            else if (strncmp(buf, "exit", 4) == 0)
            {
                // Daca comanda este de "exit", inchidem socket-ul
                close(udp_socket_fd);
                return;
            }
            else
            {
                fprintf(stderr, "Unknown command!\n");
            }
        }
        else if (poll_fds[1].revents & POLLIN)
        {
            // Daca avem date de intarare primite de la socket
            int rc = recv(poll_fds[1].fd, buf, MSG_MAX_SIZE, RECV_DEFAULT_FLAG);

            if (rc == 0)
            {
                // Daca server-ul este deconectat, inchidem socket-ul
                close(udp_socket_fd);
                return;
            }
            else
            {
                DIE(rc < 0, "Receiving data error\n");
            }

            // Adaugam caracterul null si afisam mesajul primit
            buf[rc] = '\0';
            printf("%s", buf);
        }
    }
}

int main(int argc, char *argv[])
{
    // Dezactivam buffering-ul la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Variabila pentru udp socket file descriptor
    int udp_socket_fd;
    // ID-ul clientului nou creat
    char id[ID_MAX_LEN];

    // Pornirea unui subscriber
    init_subscriber(argc, argv, udp_socket_fd, *id);
    // Rularea subscriber-ului
    run_subscriber(udp_socket_fd, id);

    return 0;
}
