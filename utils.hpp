#ifndef __UTILS__
#define __UTILS__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>

#define ID_MAX_LEN 10
#define PORT_MIN_NUB 1
#define PORT_MAX_NUM 65535
#define FD_MAX_NUM 2
#define POLL_TIMEOUT -1
#define MSG_MAX_SIZE 1500
#define RECV_DEFAULT_FLAG 0
#define CLIENTS_MAX_NUM 100

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

struct udp_msg_t {
  char topic[50];
  uint8_t type;
  char content[1500];
};

std::string process_udp_message(const udp_msg_t &msg, const std::string &ip_address, uint16_t port);
std::pair<std::string, std::string> get_topic_and_command(char cmd[]);

#endif