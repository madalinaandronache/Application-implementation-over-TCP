#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <arpa/inet.h>

#include "utils.hpp"

// Functie care proceseaza un payload de tip INT
std::string int_payload(const char content[])
{
    uint8_t sign = content[0];
    int32_t num;
    memcpy(&num, content + 1, sizeof(num));

    // Formatat conform host byte order
    num = ntohl(num);

    char value[50];
    snprintf(value, sizeof(value), "%d", sign ? -num : num);
    return std::string(value);
}

// Functie care proceseaza un payload de tip SHORT_REAL
std::string short_real_payload(const char content[])
{
    uint16_t short_real;
    memcpy(&short_real, content, sizeof(short_real));

    // Formatat conform host byte order
    short_real = ntohs(short_real);

    char value[50];

    // Modulul numarului impartit la 100
    snprintf(value, sizeof(value), "%.2f", short_real / 100.0);

    return std::string(value);
}

// Functie care proceseaza un payload de tip FLOAT
std::string float_payload(const char content[])
{
    uint8_t sign = content[0];
    uint8_t power;
    int32_t num;

    memcpy(&num, content + 1, sizeof(num));
    memcpy(&power, content + 5, sizeof(power));

    // Formatat conform host byte order
    num = ntohl(num);

    float value = num / pow(10, power);
    char value_msg[50];

    snprintf(value_msg, sizeof(value_msg), "%.4f", sign ? -value : value);
    return std::string(value_msg);
}

// Function to process STRING messages
std::string string_payload(const char content[])
{
    char value[1500];

    snprintf(value, sizeof(value), "%s", content);
    return std::string(value);
}

// Functie care transforma intr-un string un mesaj de tip UDP
std::string process_udp_message(const udp_msg_t &msg, const std::string &ip_address, uint16_t port)
{
    char message[1600];
    char type_name[20];
    std::string value;

    switch (msg.type)
    {
    case 0:
        strcpy(type_name, "INT");
        value = int_payload(msg.content);
        break;
    case 1:
        strcpy(type_name, "SHORT_REAL");
        value = short_real_payload(msg.content);
        break;
    case 2:
        strcpy(type_name, "FLOAT");
        value = float_payload(msg.content);
        break;
    case 3:
        strcpy(type_name, "STRING");
        value = string_payload(msg.content);
        break;
    }

    // Formatam mesajul <IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
    snprintf(message, sizeof(message), "%s:%u - %s - %s - %s\n", ip_address.c_str(), port, msg.topic, type_name, value.c_str());
    return std::string(message);
}

// Extragem comanda primita de la clientul TCP sub forma <Comanda> <Topic>
std::pair<std::string, std::string> get_topic_and_command(char cmd[])
{
    std::istringstream input(cmd);
    std::string command, topic;

    input >> command >> topic;

    return std::make_pair(command, topic);
}
