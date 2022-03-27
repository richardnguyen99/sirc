/**
 * @file server.c
 * @author Richard Nguyen (richard.ng0616@gmail.com)
 * @brief An oversimplified IRC server
 * @version 0.1
 * @date 2022-03-20
 *
 * @copyright Copyright (c) 2022
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <csignal>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFLEN 1024

using std::cerr;
using std::cout;
using std::endl;

/* Listening socket */
int sockfd;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void sighandler(int signo)
{
    cout << "\n[SIRC SERVER] Closing socket\n";
    if (close(sockfd) == -1)
    {
        cerr << "[SIRC SERVER] Socket cannot be closed properly. Aborted!\n";
        exit(EXIT_FAILURE);
    }

    cout << "[SIRC SERVER] Socket is closed. Exited successfully!\n";
    exit(EXIT_SUCCESS);
}

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: ./ircserver <port-number>\n";
        exit(EXIT_FAILURE);
    }

    /* New connection socket */
    int newfd;
    int status;
    int optval = 1;
    socklen_t addrlen;
    struct sockaddr_storage clientaddr;

    char remoteipv4[INET_ADDRSTRLEN];
    char buf[BUFLEN];

    /* man 3 getaddrinfo */
    struct addrinfo hints;
    struct addrinfo *results, *rp;

    fd_set master;
    fd_set rfds;
    int fdmax;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, argv[1], &hints, &results)) == -1)
    {
        cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        exit(EXIT_FAILURE);
    }

    for (rp = results; rp != NULL; rp = rp->ai_next)
    {
        if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            cerr << "socker error: " << strerror(errno) << "\n";
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
        {
            cerr << "setsockopt error: " << strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == -1)
        {
            cerr << "bind error: " << strerror(errno) << "\n";
            close(sockfd);
            continue;
        }

        /* Binding successfully */
        break;
    }

    if (rp == NULL)
    {
        cerr << "No avaiable address to bind\n";
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(results);

    cout << "[SIRC SERVER]: server is listening in port " << argv[1] << "\n";
    cout << "Press Ctrl+C to quit\n";
    if (listen(sockfd, 10) == -1)
    {
        cerr << "listen error: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sighandler) == SIG_ERR)
    {
        cerr << "signal error: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master);
    FD_SET(sockfd, &master);
    fdmax = sockfd;

    for (;;)
    {
        rfds = master;
        if (select(fdmax + 1, &rfds, NULL, NULL, NULL) == -1)
        {
            cerr << "select error: " << strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &rfds))
            {
                /* Handle new connections */
                if (i == sockfd)
                {
                    addrlen = sizeof(struct sockaddr_storage);
                    newfd = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen);

                    if (newfd == -1)
                    {
                        cerr << "accept error: " << strerror(errno) << "\n";
                        continue;
                    }

                    FD_SET(newfd, &master);
                    if (newfd > fdmax)
                        fdmax = newfd;

                    cout << "[SIRC SERVER] new connection from socket " << inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), remoteipv4, INET_ADDRSTRLEN) << "\n";

                    memset(buf, '\0', BUFLEN);
                    strcpy(buf, "A user has joined the server");
                    for (int j = 0; j <= fdmax; j++)
                    {
                        if (FD_ISSET(j, &master) && j != i && j != newfd)
                        {
                            int sentbyte;
                            if ((sentbyte = send(j, buf, strlen(buf), 0)) == -1)
                            {
                                cerr << "server fd: " << sockfd << " socket fd: " << j << "\n";
                                cerr << "send error: " << strerror(errno) << "\n";
                                continue;
                            }

                            cout << "Sent " << std::string(buf) << " - " << sentbyte << "\n";
                        }
                    }
                }
                else
                {
                    /* Handle data from a client */
                    memset(buf, '\0', BUFLEN);
                    ssize_t nbytes = recv(i, buf, BUFLEN, 0);

                    if (nbytes > 0)
                    {
                        for (int j = 0; j <= fdmax; j++)
                        {
                            if (FD_ISSET(j, &master))
                            {
                                if (j != sockfd)
                                {
                                    // if (recv(i, buf, nbytes, 0) == -1)
                                    //{
                                    // cerr << "receive error: " << strerror(errno) << "\n";
                                    // continue;
                                    //}

                                    int sentbyte;
                                    if ((sentbyte = send(j, buf, nbytes, 0)) == -1)
                                    {
                                        cerr << "send error: " << strerror(errno) << "\n";
                                        continue;
                                    }

                                    cout << "Sent " << std::string(buf) << " - " << sentbyte << "\n";
                                }
                            }
                        }

                        // cout << std::string(buf)
                        //<< "\n";

                        continue;
                    }

                    if (nbytes == 0)
                        cout << "[SIRC SERVER] socket " << i << " hung up\n";
                    else if (nbytes < 0)
                        cerr << "recv error " << strerror(errno) << "\n";

                    close(i);
                    FD_CLR(i, &master);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
