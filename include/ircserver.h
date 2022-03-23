/**
 * @file include/irc_server.h
 * @author Richard Nguyen (richard.ng0616@gmail.com)
 * @brief IRC server header
 * @version 0.1
 * @date 2022-03-19
 *
 * @copyright Copyright (c) 2022
 */

#ifndef _IRC_SERVER_H
#define _IRC_SERVER_H 1

/* For I/O input and output */
#include <stdio.h>  // fprintf, stderr, stdout
#include <string.h> // memset, strlen
#include <stdlib.h> // NULL, exit, malloc, free, EXIT_*

/* For socket */
#include <sys/types.h>
#include <sys/socket.h> // socket, setsockopt, AF_*, SOCK_*, AI_*, SOL_*, SO_*, socklen_t, sockaddr_storage
#include <netdb.h>      // getaddrinfo, addrinfo, gai_strerror, freeaddrinfo

/* For operating system */
#include <sys/wait.h> // waitpid
#include <signal.h>   // sigaction, sigemptyset, SA_*
#include <errno.h>    // errno
#include <pthread.h>  // phthread_create
#include <unistd.h>   // close

#define BUFLEN 256
#define CRLF "\r\n"
#define BACKLOG 10

typedef struct _client_arg_s
{
    int fd;
    struct sockaddr_storage addr;
} client_arg_t;

typedef struct _client_list_s
{
    client_arg_t client;
    struct _client_list_s *next;
} client_list_t;

void readfrom(int socket, char **msg)
{
    char buffer[BUFLEN];
    *msg = (char *)malloc(sizeof(char) * BUFLEN);

    int rbyte = 0;

    for (;;)
    {
        rbyte = recv(socket, (void *)buffer, BUFLEN, 0);

        if (rbyte == -1)
        {
            perror("recv error");
            break;
        }

        buffer[rbyte] = '\0';
        strcat(buffer, *msg);

        if ((BUFLEN - strlen(*msg)) < rbyte)
        {
            *msg = (char *)realloc(*msg, strlen(*msg) + sizeof(char) * BUFLEN);
        }
        strcpy(buffer, *msg);
    }

    if ((BUFLEN - strlen(*msg)) < rbyte)
    {
        *msg = (char *)realloc(*msg, strlen(*msg) + sizeof(char) * BUFLEN);
    }
    strcpy(buffer, *msg);
}

/**
 * @brief Establish a new connection
 *
 * @param arg client_arg_t
 */
void *establish(void *arg)
{
    client_arg_t *client_arg = (client_arg_t *)arg;

    int clientfd = client_arg->fd;
    struct sockaddr_storage addrlen = client_arg->addr;

    free(arg);

    for (;;)
    {
        char *msg;
        readfrom(clientfd, &msg);

        fprintf(stdout, "%s\n", msg);

        free(msg);
    }
}

#endif // _IRC_SERVER_H
