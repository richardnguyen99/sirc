/**
 * @file helper.h
 * @author Richard Nguyen (richard.ng0616@gmail.com)
 * @brief Helper functions for IRC
 * @version 0.1
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022
 */

#ifndef _IRC_HELPER
#define _IRC_HELPER 1

#include <stdlib.h>
#include "ircserver.h"

void *enqueue(client_list_t **root, client_list_t **head, client_list_t **tail, client_arg_t *client)
{
    if (root == NULL)
    {
        *root = (client_list_t *)malloc(sizeof(client_list_t));
        (*root)->client.fd = client->fd;
        (*root)->client.addr = client->addr;
        (*root)->next = NULL;

        *head = *root;
        *tail = *root;
    }
    else
    {
        (*tail)->next = (client_list_t *)malloc(sizeof(client_list_t));
        (*tail)->next->client = *client;
        (*tail)->next->next = NULL;

        *tail = (*tail)->next;
    }
}

#endif /* _IRC_HELPER */
