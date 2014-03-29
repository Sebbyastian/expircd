#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include "sock.h"

struct casemap;
struct node;
struct nodeinfo;

typedef int evaluator(struct node *, struct nodeinfo **);

typedef struct casemap {
    char *description;
    int (*tolower)(int);
} casemap;

typedef struct evaluatorinfo {
    char *name;
    evaluator *evaluate;
} evaluatorinfo;

typedef struct node {
    char nickname[NICKLEN];
    size_t offset;
    evaluator *evaluate;

    union {
        size_t offset;
        struct node *node;
    } source;

    union {
        size_t offset;
        struct node *node;
    } target;

    union {
        size_t offset[2];
        struct node *node[2];
    } next;

    union {
        struct { /* only valid when evaluate is set to user_* functions (except for user_channel) */
            sockfd fd;

            union {
                size_t offset;
                struct node *node;
            } first_channel;

            size_t recvdata_mark;
            size_t recvdata_size;
            int    recvdata_past;
            size_t senddata_size;

            char recvdata[512];
            char username[USERLEN];
            char hostname[HOSTLEN];
        };

        struct { /* only valid when evaluate is set to channel_info */
            char topic[TOPICLEN];
            char key[KEYLEN];
            size_t limit;
            union {
               size_t offset;
               struct node *node;
            } first_user;
            unsigned int invite_only   :1,
                         moderate      :1,
                         private       :1,
                         secret        :1,
                         topic_restrict:1;
        };

        struct { /* only valid when evaluate is set to channel_user */
            union {
               size_t offset;
               struct node *node;
            } next_user;
            union {
               size_t offset;
               struct node *node;
            } user[];
        };

        struct { /* only valid when evaluate is set to user_channel */
            union {
               size_t offset;
               struct node *node;
            } next_channel;
            union {
               size_t offset;
               struct node *node;
            } channel[];
        };
    };
} node;

typedef struct nodeinfo {
    size_t size;
    union {
        size_t offset;
        node *node;
    } root;
    node node[];
} nodeinfo;

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int ascii_tolower(int);
int strict_rfc1459_tolower(int);
int rfc1459_tolower(int);

int channel_info(node *, nodeinfo **);
int channel_user(node *, nodeinfo **);

size_t node_bit(void *, size_t, size_t);
int node_cleanup(node *, nodeinfo **);
size_t node_compare(void *, void *, size_t, size_t);

node *nodeinfo_add(nodeinfo **, node *);
void nodeinfo_bind(nodeinfo **, addrinfo *, evaluator *);
node *nodeinfo_get(nodeinfo **, void *, size_t);
node **nodeinfo_getref(nodeinfo **, void *, size_t);

int evaluatorinfo_compare(const void *, const void *);

int server_accept(node *, nodeinfo **);

int user_channel(node *, nodeinfo **);
int user_discard(node *);
int user_discard_line(node *);
int user_error(node *, nodeinfo **, evaluator *, char *);
int user_handle(node *, nodeinfo **, evaluatorinfo *, size_t, evaluator *, evaluator *);
int user_nickname(node *, nodeinfo **, evaluator *, evaluator *);
int user_nickname_success(node *, nodeinfo **, evaluator *);
int user_participation(node *, nodeinfo **);
int user_participation_discard_line(node *, nodeinfo **);
int user_participation_join(node *, nodeinfo **);
int user_participation_nickname(node *, nodeinfo **);
int user_participation_nickname_success(node *, nodeinfo **);
int user_participation_nickname_in_use(node *, nodeinfo **);
int user_participation_no_such_entity(node *, nodeinfo **);
int user_participation_not_enough_parameters(node *, nodeinfo **);
int user_participation_message(node *, nodeinfo **, evaluator *);
int user_participation_notice(node *, nodeinfo **);
int user_participation_notice_handler(node *, nodeinfo **);
int user_participation_privmsg(node *, nodeinfo **);
int user_participation_privmsg_handler(node *, nodeinfo **);
int user_participation_relay_header(node *, nodeinfo **, char *);
int user_participation_relay_message(node *, nodeinfo **);
int user_participation_unknown_command(node *, nodeinfo **);
int user_participation_username(node *, nodeinfo **);
int user_participation_welcome(node *, nodeinfo **);
int user_recv(node *);
int user_registration(node *, nodeinfo **);
int user_registration_discard_line(node *, nodeinfo **);
int user_registration_nickname(node *, nodeinfo **);
int user_registration_nickname_success(node *, nodeinfo **);
int user_registration_nickname_in_use(node *, nodeinfo **);
int user_registration_not_enough_parameters(node *, nodeinfo **);
int user_participation_notice(node *, nodeinfo **);
int user_participation_privmsg(node *, nodeinfo **);
int user_registration_unknown_command(node *, nodeinfo **);
int user_registration_username(node *, nodeinfo **);
int user_send(node *, char *, size_t);
int sendf(node *, char *, ...);
#endif