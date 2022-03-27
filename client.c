#define _GNU_SOURCE

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#define BUFLEN 1024

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int sendmsgto(int sockfd, const char *msg)
{
    int msglen = strlen(msg);
    int sentbyte = 0;

    while (sentbyte < msglen)
    {
        int byte = send(sockfd, (void *)msg, msglen - sentbyte, 0);

        if (byte == -1)
        {
            // fprintf(stderr, "send error %s\n", strerror(errno));
            return -1;
        }

        sentbyte += byte;
    }

    return 0;
}

int readmsgfrom(int sockfd, char *msg)
{
    // int rbyte = 0;

    int byte = recv(sockfd, msg, BUFLEN, 0);

    if (byte < 0)
    {
        // fprintf(stderr, "");
        return -1;
    }

    msg[byte] = '\0';

    return 0;
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./ircclient <hostname> <port number>\n");
        // exit(EXIT_FAILURE);
    }

    // Special strings for validation
    const char special[] = "!@#$%%^&*()_+\\`~_=[]{}|;':\",./<>? ";
    const char quit[] = "quit";

    int parent_x, parent_y, new_x, new_y;
    int chatinwin_size = 3;
    int key = ERR;
    int row = 0, line = 0;
    int i = 0;
    int viscnt = 0;

    fd_set master;
    fd_set rfds;
    int fdmax;

    int status;
    int sockfd;
    char remoteipv4[INET_ADDRSTRLEN];

    struct addrinfo hints;
    struct addrinfo *results = NULL, *rp = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], argv[2], &hints, &results)) < 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    for (rp = results; rp != NULL; rp = rp->ai_next)
    {
        if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            fprintf(stderr, "socket error: %s\n", strerror(errno));
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == -1)
        {
            close(sockfd);
            fprintf(stderr, "connect error: %s\n", strerror(errno));
            continue;
        }

        break;
    }

    if (rp == NULL)
    {
        fprintf(stderr, "No available address to connect to\n");
        exit(EXIT_FAILURE);
    }

    // Store IPv4 address
    inet_ntop(rp->ai_family, get_in_addr((struct sockaddr *)rp->ai_addr), remoteipv4, sizeof(remoteipv4));

    freeaddrinfo(results);

    char tmpbuf[BUFLEN];
    char buffer[BUFLEN];
    memset(tmpbuf, 0, BUFLEN);
    memset(buffer, 0, BUFLEN);

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    // set up initial windows
    getmaxyx(stdscr, parent_y, parent_x);
    char **log = malloc(sizeof(buffer) * parent_y);
    sprintf(buffer, "Connected to %s", remoteipv4);
    log[i++] = strdup(buffer);

    WINDOW *chatlogwin = newwin(parent_y - chatinwin_size, parent_x, 0, 0);
    WINDOW *chatinwin = newwin(chatinwin_size, parent_x, parent_y - chatinwin_size, 0);

    FD_ZERO(&master);
    FD_SET(STDIN_FILENO, &master);
    FD_SET(sockfd, &master);
    fdmax = sockfd;

    while (1)
    {

        box(chatlogwin, 0, 0);
        box(chatinwin, 0, 0);
        if (key == KEY_RESIZE)
        {
#ifdef PDCURSES
            resize_term(0, 0);
#endif
            getmaxyx(stdscr, new_y, new_x);
            size_t arrlen = (int)sizeof(log) / (int)sizeof(log[0]);
            if (row >= arrlen)
            {
                log = realloc(log, sizeof(buffer) * arrlen * 2);
                continue;
            }

            if (new_y != parent_y || new_x != parent_x)
            {
                parent_x = new_x;
                parent_y = new_y;

                wresize(chatlogwin, new_y - chatinwin_size, new_x);
                wresize(chatinwin, chatinwin_size, new_x);
                mvwin(chatinwin, new_y - chatinwin_size, 0);

                wclear(stdscr);
                wclear(chatlogwin);
                wclear(chatinwin);

                box(chatlogwin, 0, 0);
                box(chatinwin, 0, 0);
            }
        }

        // draw to our windows
        mvwprintw(chatlogwin, 0, 1, "CHAT LOG");
        for (int j = 0; j < (i - viscnt); j++)
            mvwprintw(chatlogwin, j + 1, 1, "%s", log[viscnt + j]);
        mvwprintw(chatinwin, 0, 1, "CHAT INPUT");
        mvwprintw(chatinwin, 1, 1, "%s", tmpbuf);

        // refresh each window
        wrefresh(stdscr);
        wrefresh(chatlogwin);
        wrefresh(chatinwin);

        rfds = master;
        status = select(fdmax + 1, &rfds, NULL, NULL, NULL);

        if (status < 0)
        {
            fprintf(stderr, "select error %s\n", strerror(errno));
            break;
        }

        for (int k = 0; k <= fdmax; k++)
        {
            if (!FD_ISSET(k, &rfds))
                continue;

            if (k == sockfd)
            {
                char recvmsg[BUFLEN];
                status = readmsgfrom(k, recvmsg);
                log[i++] = strdup(recvmsg);
                if (i >= (parent_y - chatinwin_size - 1))
                    viscnt++;
            }

            else if (k == STDIN_FILENO)
            {
                key = getch();

                if (isalpha(key) || isdigit(key) || strchr(special, (int)key))
                {
                    char tempkstr[2];
                    tempkstr[0] = key;
                    tempkstr[1] = '\0';

                    if (strlen(tmpbuf) < BUFLEN)
                        strcat(tmpbuf, tempkstr);
                    line++;
                }
                else if (key == KEY_BACKSPACE)
                {
                    tmpbuf[--line] = '\0';
                }
                else if (key == '\n')
                {
                    if (strlen(tmpbuf) == 0)
                        continue;

                    if (!strcmp(tmpbuf, quit))
                        goto endclient;

                    strcpy(buffer, tmpbuf);
                    status = sendmsgto(sockfd, buffer);

                    if (status == -1)
                    {
                        log[i++] = "Couldn't send the message";
                        if (i >= (parent_y - chatinwin_size - 1))
                            viscnt++;
                    }

                    row++;    // Move to the next row
                    line = 0; // Reset cursor position in a row
                    memset(tmpbuf, 0, BUFLEN);
                    memset(buffer, 0, BUFLEN);
                }
            }
        }

        // refresh each window
        wclear(stdscr);
        wclear(chatlogwin);
        wclear(chatinwin);
    }

endclient:
    free(log);
    endwin();
    close(sockfd);

    return 0;
}
