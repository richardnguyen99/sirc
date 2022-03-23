/**
 * @file client.cpp
 * @author Richard Nguyen (richard.ng0616@gmail.com)
 * @brief A very simple IRC client
 * @version 0.1
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cerrno>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFLEN 1024

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using namespace std::string_literals;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int sendmsgto(int sockfd, const std::string &msg)
{
    const char *buffer = msg.c_str();
    int msglen = msg.length();
    int sentbyte = 0;
    int count = 0;

    while (sentbyte < msglen)
    {
        count++;
        int byte = send(sockfd, (void *)(buffer + sentbyte), msglen - sentbyte, 0);

        if (byte == -1)
        {
            cerr << "send error " << strerror(errno) << "\n";
            return -1;
        }

        sentbyte += byte;
    }
    cout << "Sent: " << count
         << "\n";
    return 0;
}

int main(int argc, const char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: ./ircclient <hostname> <port>\n";
        exit(EXIT_FAILURE);
    }

    int sockfd;
    int status;
    // char buf[BUFLEN];
    char remoteipv4[INET_ADDRSTRLEN];

    struct addrinfo hints;
    struct addrinfo *results, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], argv[2], &hints, &results)) == -1)
    {
        cerr << "getaddrinfo error: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    for (rp = results; rp != NULL; rp = rp->ai_next)
    {
        if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            cerr << "socket error: " << strerror(errno) << "\n";
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == -1)
        {
            close(sockfd);
            cerr << "connect error: " << strerror(errno) << "\n";
            continue;
        }

        break;
    }

    if (rp == NULL)
    {
        cerr << "No available address to connect\n";
        exit(EXIT_FAILURE);
    }

    inet_ntop(rp->ai_family, get_in_addr((struct sockaddr *)rp->ai_addr), remoteipv4, sizeof(remoteipv4));
    cout << "[SIRC CLIENT] Connected to host " << remoteipv4 << "\n";

    freeaddrinfo(results);

    std::string input;

    status = 0;
    for (; status != -1;)
    {
        cout << "[SIRC CLIENT]> ";
        std::getline(cin, input);

        status = sendmsgto(sockfd, input);
    }

    cout << "[SIRC CLIENT] Closing socket\n";
    if (close(sockfd) == -1)
    {
        cerr << "[SIRC CLIENT] Socket couldnot be closed properly. Aborted!\n";
        exit(EXIT_FAILURE);
    }
    cout << "[SIRC CLIENT] Socket is closed. Exited successfully\n";
    exit(0);
}
