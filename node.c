#include "node.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define debug(statement) (printf("%s:%u entered\n", __FILE__, (unsigned int) __LINE__), statement); printf("%s:%u exited\n", __FILE__, (unsigned int) __LINE__)

casemap ascii = { "ascii", ascii_tolower };
casemap strict_rfc1459 = { "strict-rfc1459", strict_rfc1459_tolower };
casemap rfc1459 = { "rfc1459", rfc1459_tolower };

/* XXX: These need to be changed because IRC guarantees ASCII, where-as C makes no such guarantee.
 *      Reference: http://www.irc.org/tech_docs/005.html */

int ascii_tolower(int c) {
    return tolower(c);
}

int strict_rfc1459_tolower(int c) {
    const char *punc = "[]\\";
    return strchr(punc, c) ? "{}|"[strchr(punc, c) - punc] : ascii_tolower(c);
}

int rfc1459_tolower(int c) {
    return c == '^' ? '~' : strict_rfc1459_tolower(c);
}

/* END XXX */

int channel_info(node *c, nodeinfo **list) {
    return 0;
}

int channel_user(node *c, nodeinfo **list) {
    return 0;
}

size_t node_bit(void *u, size_t offset, size_t size) {
    unsigned char *u_nickname = u;
    size_t hi = offset / CHAR_BIT;
    return hi < size && (CASEMAPPING.tolower(u_nickname[hi]) >> (~offset % CHAR_BIT)) % 2;
}

int node_cleanup(node *u, nodeinfo **list) {
    assert(0);
    return 0;
}

size_t node_compare(void *x, void *y, size_t offset, size_t size) {
    unsigned char *x_nickname = x,
                  *y_nickname = y;
    size_t hi = offset / CHAR_BIT;

    while (hi < size && CASEMAPPING.tolower(x_nickname[hi]) == CASEMAPPING.tolower(y_nickname[hi])) {
        hi++;
    }

    size_t lo = hi > offset / CHAR_BIT
              ? ~0U % CHAR_BIT
              : ~offset % CHAR_BIT,
           sum = hi < size
               ? CASEMAPPING.tolower(x_nickname[hi]) ^ CASEMAPPING.tolower(y_nickname[hi])
               : ~0U % UCHAR_MAX;

    while (lo > 0 && sum >> lo == 0) {
        lo--;
    }

    return hi * CHAR_BIT + ~lo % CHAR_BIT;
}

node *nodeinfo_add(nodeinfo **list, node *u) {
    size_t old_size = *list ? (*list)->size : 0, new_size = old_size + 1;
    assert(new_size > old_size);

    if ((old_size & new_size) == 0) {
        int tree_formed = old_size && (*list)->root.node;
        node *l = tree_formed ? (*list)->node : NULL;

        if (SIZE_MAX / 2 / sizeof *u <= new_size) { return NULL; }

        if (tree_formed) {
            (*list)->root.offset = (*list)->root.node - (*list)->node;
            for (size_t x = 0; x < old_size; x++) {
                node *n = l + x;

                size_t next[2] = { n->next.node[0] ? n->next.node[0] - l : 0,
                                   n->next.node[1] ? n->next.node[1] - l : 0 };
                n->next.offset[0] = next[0];
                n->next.offset[1] = next[1];

                if (n->evaluate == channel_user) {
                    n->next_user.offset = n->next_user.node ? n->next_user.node - l : 0;

                    for (size_t x = 0; x < (sizeof *n - offsetof(node, user)) / sizeof *(n->user); x++) {
                        n->user[x].offset = n->user[x].node ? n->user[x].node - l : 0;
                    }

                    continue;
                }

                if (n->evaluate == user_channel) {
                    n->next_channel.offset = n->next_channel.node ? n->next_channel.node - l : 0;

                    for (size_t x = 0; x < (sizeof *n - offsetof(node, channel)) / sizeof *(n->channel); x++) {
                        n->channel[x].offset = n->channel[x].node ? n->channel[x].node - l : 0;
                    }

                    continue;
                }

                size_t source = n->source.node ? n->source.node - l : 0;
                size_t target = n->target.node ? n->target.node - l : 0;

                n->source.offset = source;
                n->target.offset = target;

                if (n->evaluate == channel_info) {
                    n->first_user.offset = n->first_user.node ? n->first_user.node - l : 0;
                    continue;
                }

                n->first_channel.offset = n->first_channel.node ? n->first_channel.node - l : 0;
            }
        }

        size_t new_capacity = new_size * 2 * sizeof *u;
        assert(new_capacity > new_size);

        void *temp = realloc(*list, sizeof *list + new_capacity);
        if (temp == NULL) { return NULL; }

        *list = temp;
        if (!tree_formed) {
            (*list)->root.node = NULL;
        }
        else {
            l = (*list)->node;
            (*list)->root.node = (*list)->node + (*list)->root.offset;
            for (size_t x = 0; x < old_size; x++) {
                node *n = l + x;

                node *next[2] = { n->next.offset[0] ? l + n->next.offset[0] : NULL,
                                  n->next.offset[1] ? l + n->next.offset[1] : NULL };
                n->next.node[0] = next[0];
                n->next.node[1] = next[1];

                if (n->evaluate == channel_user) {
                    n->next_user.node = n->next_user.offset ? l + n->next_user.offset : NULL;

                    for (size_t x = 0; x < (sizeof *n - offsetof(node, user)) / sizeof *(n->user); x++) {
                        n->user[x].node = n->user[x].offset ? l + n->user[x].offset : NULL;
                    }

                    continue;
                }

                if (n->evaluate == user_channel) {
                    n->next_channel.node = n->next_channel.offset ? l + n->next_channel.offset : NULL;

                    for (size_t x = 0; x < (sizeof *n - offsetof(node, channel)) / sizeof *(n->channel); x++) {
                        n->channel[x].node = n->channel[x].offset ? l + n->channel[x].offset : NULL;
                    }

                    continue;
                }

                node *source = n->source.offset ? l + n->source.offset : NULL;
                node *target = n->target.offset ? l + n->target.offset : NULL;

                n->source.node = source;
                n->target.node = target;

                if (n->evaluate == channel_info) {
                    n->first_user.node = n->first_user.offset ? l + n->first_user.offset : NULL;
                    continue;
                }

                n->first_channel.node = n->first_channel.offset ? l + n->first_channel.offset : NULL;
            }
        }
    }

    (*list)->size = new_size;
    (*list)->node[old_size] = *u;

    u = (*list)->node + old_size;
    u->next.node[0] = u;
    u->next.node[1] = u;
    return u;
}

void nodeinfo_bind(nodeinfo **list, addrinfo *addr, evaluator *e) {
    while (addr != NULL) {
        sockfd fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (sock_invalid(fd)) {
            addr = addr->ai_next;
            continue;
        }

        if (!listen(fd, addr) || !set_nonblock(fd) || !nodeinfo_add(list, &(node){ .fd = fd,
                                                                                   .evaluate = e })) {
            closesocket(fd);
        }

        addr = addr->ai_next;
    }
}

node *nodeinfo_get(nodeinfo **list, void *u, size_t size) {
    node *n = *nodeinfo_getref(list, u, size);

    return n != NULL && node_compare(n, u, 0, size) / CHAR_BIT == size ? n : NULL;
}

node **nodeinfo_getref(nodeinfo **list, void *u, size_t size) {
    size_t offset;
    node **n = &(*list)->root.node;

    if (*n == NULL) {
        return n;
    }

    do {
        offset = (*n)->offset;
        n = (*n)->next.node + node_bit(u, offset, size);
    } while (offset < (*n)->offset);

    return n;
}

int evaluatorinfo_compare(const void *x, const void *y) {
    return strcmp(((const evaluatorinfo *) x)->name, ((const evaluatorinfo *) y)->name);
}

int server_accept(node *u, nodeinfo **list) {
    size_t count = 0;
    sockfd fd = accept(u->fd);

    if (sock_invalid(fd)) {
        return 0;
    }

    if (!set_nonblock(fd) || !nodeinfo_add(list, &(node){ .fd = fd,
                                                          .evaluate = user_registration })) {
        closesocket(fd);
    }

    return 0;
}

int user_channel(node *u, nodeinfo **list) {
    return 0;
}

int user_discard(node *u) {
    u->recvdata_past = u->recvdata[u->recvdata_mark++];
    u->recvdata_size -= u->recvdata_mark;
    memmove(u->recvdata, u->recvdata + u->recvdata_mark, u->recvdata_size);
    u->recvdata_mark = 0;
    return u->recvdata_past;
}

int user_discard_line(node *u) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }
    return user_discard(u) != ' ';
}

int user_error(node *u, nodeinfo **list, evaluator *e, char *format) {
    int n = sendf(u, format, HOSTNAME, NICKLEN, u->nickname[0] == '\0' ? "*" : u->nickname, u->recvdata_mark, u->recvdata);
    if (n <= 0) {
        return n;
    }

    u->evaluate = e;
    return u->evaluate(u, list);
}

int user_evaluate(node *u, nodeinfo **list, evaluatorinfo *e, size_t e_size, evaluator *not_enough_parameters, evaluator *unknown_command) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    if (u->recvdata[u->recvdata_mark] != ' ') {
        u->evaluate = not_enough_parameters;
        return u->evaluate(u, list);
    }

    u->recvdata[u->recvdata_mark] = '\0';
    e = bsearch(&(evaluatorinfo){ .name = u->recvdata }, e, e_size, sizeof *e, evaluatorinfo_compare);
    u->recvdata[u->recvdata_mark] = ' ';

    if (e == NULL) {
        u->evaluate = unknown_command;
        return u->evaluate(u, list);
    }

    u->evaluate = e->evaluate;
    user_discard(u);
    return u->evaluate(u, list);
}

int user_nickname(node *u, nodeinfo **list, evaluator *nickname_success, evaluator *nickname_in_use) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    node *v = nodeinfo_get(list, u->recvdata, u->recvdata_mark);
    u->evaluate = v && v != u
                ? nickname_in_use
                : nickname_success;
    return u->evaluate(u, list);
}

int user_nickname_success(node *u, nodeinfo **list, evaluator *e) {
    size_t nickname_size = u->recvdata_mark < NICKLEN ? u->recvdata_mark : NICKLEN;
    size_t child_offset, parent_offset = 0;
    node *root = (*list)->root.node ? (*list)->root.node : u, **branch = &root;

    u->evaluate = e;
    memmove(u->nickname, u->recvdata, nickname_size);
    memset(u->nickname + nickname_size, 0, NICKLEN - nickname_size);

    do {
        child_offset = node_compare(*branch, u, parent_offset, NICKLEN);
        parent_offset = (*branch)->offset;

        if (child_offset < parent_offset) {
            break;
        }

        branch = (*branch)->next.node + node_bit(u, parent_offset, NICKLEN);
    } while ((*branch)->offset > parent_offset);

    if (child_offset >= parent_offset) {
        child_offset = node_compare(*branch, u, parent_offset, NICKLEN);
    }

    size_t bit = node_bit(*branch, child_offset, NICKLEN);
    assert(bit == 0 || bit == 1);

    u->offset = child_offset;
    u->next.node[bit-0] = *branch;
    u->next.node[1-bit] = u;

    *branch = u;
    (*list)->root.node = root;
    return u->evaluate(u, list);
}

int user_participation(node *u, nodeinfo **list) {
    static evaluatorinfo e[] = { { .name = "NICK", .evaluate = user_participation_nickname },
                                 { .name = "NOTICE", .evaluate = user_participation_notice },
                                 { .name = "PRIVMSG", .evaluate = user_participation_privmsg },
                                 { .name = "USER", .evaluate = user_participation_username } };
    static size_t unsorted = sizeof e / sizeof *e;
    qsort(e, unsorted, sizeof *e, evaluatorinfo_compare);
    unsorted = 0;

    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    return user_evaluate(u, list, e, sizeof e / sizeof *e, user_participation_not_enough_parameters, user_participation_unknown_command);
}

int user_participation_discard_line(node *u, nodeinfo **list) {
    int n = user_discard_line(u);
    if (n <= 0) {
        return n;
    }

    u->evaluate = user_participation;
    return u->evaluate(u, list);
}

int user_participation_error(node *u, nodeinfo **list, char *format) {
    return user_error(u, list, user_participation_discard_line, format);
}

int user_participation_join(node *u, nodeinfo **list) {
    assert(0);
    return 0;
}

int user_participation_nickname(node *u, nodeinfo **list) {
    return user_nickname(u, list, user_participation_nickname_success, user_participation_nickname_in_use);
}

int user_participation_nickname_success(node *u, nodeinfo **list) {
    size_t nickname_size = u->recvdata_mark < NICKLEN ? u->recvdata_mark : NICKLEN;
    int n = sendf(u, ":%.*s NICK :%.*s\r\n", NICKLEN, u->nickname, nickname_size, u->recvdata);
    if (n <= 0) {
        return n;
    }

    size_t x;
    while (x < nickname_size && CASEMAPPING.tolower(u->nickname[x]) != CASEMAPPING.tolower(u->recvdata[x])) {
        x++;
    }

    if (x == nickname_size) {
        memmove(u->nickname, u->recvdata, nickname_size);
        memset(u->nickname + nickname_size, 0, NICKLEN - nickname_size);
        u->evaluate = user_participation_discard_line;
        return u->evaluate(u, list);
    }

    node **v = nodeinfo_getref(list, u->nickname, NICKLEN);
    do {
        *v = (*v)->next.node[0];
    } while ((*v)->next.node[0] != u);

    node *w = (*v)->next.node[1];
    (*v)->next.node[0] = u->next.node[0];
    (*v)->next.node[1] = u->next.node[1];
    *v = w;

    return user_nickname_success(u, list, user_participation_discard_line);
}

int user_participation_nickname_in_use(node *u, nodeinfo **list) {
    return user_participation_error(u, list, ":%s 433 %.*s %.*s :Nickname is already in use\r\n");
}

int user_participation_no_such_entity(node *u, nodeinfo **list) {
    return user_participation_error(u, list, ":%s 401 %.*s %.*s :No such nick/channel\r\n");
}

int user_participation_not_enough_parameters(node *u, nodeinfo **list) {
    return user_participation_error(u, list, ":%s 461 %.*s %.*s :Not enough parameters\r\n");
}

int user_participation_message(node *u, nodeinfo **list, evaluator *e) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    node *t = nodeinfo_get(list, u->recvdata, u->recvdata_mark);
    if (t == NULL) {
        u->evaluate = user_participation_no_such_entity;
        return u->evaluate(u, list);
    }

    if (user_discard(u) != ' ') {
        u->evaluate = user_participation;
        return 1;
    }

    u->target.node = t;
    u->evaluate = e;
    return u->evaluate(u, list);
}

int user_participation_notice(node *u, nodeinfo **list) {
    return debug(user_participation_message(u, list, user_participation_notice_handler));
}

int user_participation_notice_handler(node *u, nodeinfo **list) {
    return user_participation_relay_header(u, list, "NOTICE");
}

int user_participation_privmsg(node *u, nodeinfo **list) {
    return user_participation_message(u, list, user_participation_privmsg_handler);
}

int user_participation_privmsg_handler(node *u, nodeinfo **list) {
    return user_participation_relay_header(u, list, "PRIVMSG");
}

int user_participation_relay_header(node *u, nodeinfo **list, char *action) {
    node *t = u->target.node;
    if (t->source.node && t->source.node != u) {
        return 1;
    }

    t->source.node = u;

    int n = sendf(t, ":%.*s!%.*s@%.*s %s %.*s :", NICKLEN, u->nickname, USERLEN, u->username, HOSTLEN, u->hostname, action, NICKLEN, t->nickname);
    if (n <= 0) {
        return 1;
    }

    u->evaluate = user_participation_relay_message;
    return u->evaluate(u, list);
}

int user_participation_relay_message(node *u, nodeinfo **list) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    node *t = u->target.node;
    t->source.node = u;

    n = user_send(t, u->recvdata, u->recvdata_size);
    if (n <= 0) {
        return 1;
    }

    user_discard(u);
    u->evaluate = user_participation;
    t->source.node = NULL;
    return 1;
}

int user_participation_unknown_command(node *u, nodeinfo **list) {
    return user_participation_error(u, list, ":%s 421 %.*s %.*s :Unknown command\r\n");
}

int user_participation_username(node *u, nodeinfo **list) {
    return user_participation_error(u, list, ":%s 421 %.*s %.*s :You may not reregister\r\n");
}

int user_participation_welcome(node *u, nodeinfo **list) {
    int n = sendf(u, ":%s 001 %.*s :Welcome to the Internet Relay Network %.*s!%.*s@%.*s\r\n", HOSTNAME, NICKLEN, u->nickname,
                                                                                                         NICKLEN, u->nickname,
                                                                                                         USERLEN, u->username,
                                                                                                         HOSTLEN, u->hostname);
    if (n <= 0) {
        return n;
    }

    u->evaluate = user_participation;
    return 1;
}

int user_recv(node *u) {
    int n = recv(u->fd, u->recvdata + u->recvdata_size, sizeof u->recvdata - u->recvdata_size, 0);
    if (n < 0) {
        switch (sock_again(fd)) {
            case 0: return n;
            case 1: n = 0;
                    break;
        }
    }

    int past = u->recvdata_past;
    if (past == ' ' && u->recvdata[0] == ':') {
        past = user_discard(u);
    }

    for (u->recvdata_size += n; u->recvdata_mark < u->recvdata_size; u->recvdata_mark++) {
        if (memchr(" \r\n\0" + (past == ':'), u->recvdata[u->recvdata_mark], 4 - (past == ':')) != NULL) {
            return 1;
        }
    }

end:return u->recvdata_mark < sizeof u->recvdata ? 0 : -1;
}

int user_registration(node *u, nodeinfo **list) {
    static evaluatorinfo e[] = { { .name = "NICK", .evaluate = user_registration_nickname },
                                 { .name = "USER", .evaluate = user_registration_username } };
    static size_t unsorted = sizeof e / sizeof *e;
    qsort(e, unsorted, sizeof *e, evaluatorinfo_compare);
    unsorted = 0;

    return user_evaluate(u, list, e, sizeof e / sizeof *e, user_registration_not_enough_parameters, user_registration_unknown_command);
}

int user_registration_discard_line(node *u, nodeinfo **list) {
    int n = user_discard_line(u);
    if (n <= 0) {
        return n;
    }

    u->evaluate = u->nickname[0] == '\0' || u->username[0] == '\0'
                ? user_registration
                : user_participation_welcome;
    return u->evaluate(u, list);
}

int user_registration_error(node *u, nodeinfo **list, char *format) {
    return user_error(u, list, user_registration_discard_line, format);
}

int user_registration_nickname(node *u, nodeinfo **list) {
    return user_nickname(u, list, user_registration_nickname_success, user_registration_nickname_in_use);
}

int user_registration_nickname_success(node *u, nodeinfo **list) {
    return user_nickname_success(u, list, user_registration_discard_line);
}

int user_registration_nickname_in_use(node *u, nodeinfo **list) {
    return user_registration_error(u, list, ":%s 433 %.*s %.*s :Nickname is already in use\r\n");
}

int user_registration_not_enough_parameters(node *u, nodeinfo **list) {
    return user_registration_error(u, list, ":%s 461 %.*s %.*s :Not enough parameters\r\n");
}

int user_registration_unknown_command(node *u, nodeinfo **list) {
    return user_registration_error(u, list, ":%s 421 %.*s %.*s :Unknown command\r\n");
}

int user_registration_username(node *u, nodeinfo **list) {
    int n = user_recv(u);
    if (n <= 0) {
        return n;
    }

    size_t username_size = u->recvdata_mark < USERLEN ? u->recvdata_mark : USERLEN;
    assert(username_size > 0);

    memmove(u->username, u->recvdata, username_size);
    memset(u->username + username_size, 0, USERLEN - username_size);

    struct sockaddr name;

    n = sock_invalid(getpeername(u->fd, &name, (socklen_t[]) { sizeof name }));
    assert(n == 0);

    n = getnameinfo(&name, sizeof name, u->hostname, HOSTLEN, NULL, 0, NI_NUMERICHOST);
    assert(n == 0);

    u->evaluate = user_registration_discard_line;
    return u->evaluate(u, list);
}

int channel_send(node *c, char *data, size_t size) {
    for (node *target = c->target.node ? c->target.node : c->first_user.node; target != NULL; target = target->next_user.node) {
        while (target->target.offset < (sizeof *target - offsetof(node, user)) / sizeof *(target->user)) {
            node *u = target->user[target->target.offset].node;
            if (!u) {
                target->target.offset++;
                continue;
            }

            if (u->source.node && u->source.node != target) {
                c->target.node = target;
                return 0;
            }

            u->source.node = target;

            int n = user_send(u, data, size);
            if (n < 0) {
                c->target.node = target;
                return n;
            }
            target->target.offset++;
        }
        target->target.offset = 0;
    }
    c->target.node = NULL;
    return 1;
}

int user_send(node *u, char *data, size_t size) {
    if (u->senddata_size >= size) {
        u->senddata_size = 0;
        return 1;
    }

    int n = send(u->fd, data + u->senddata_size, size - u->senddata_size <= INT_MAX ? size - u->senddata_size : INT_MAX, 0);
    if (n < 0) {
        switch (sock_again(n)) {
            case 0: return n;
            case 1: n = 0;
                    break;
        }
    }

    u->senddata_size += n;
    if (u->senddata_size < size) {
        return 0;
    }

    u->senddata_size = 0;
    return 1;
}

int sendf(node *n, char *format, ...) {
    va_list list, copy;
    va_start(list, format);
    va_copy(copy, list);

    char data[vsnprintf(NULL, 0, format, copy) + 1];
    vsnprintf(data, sizeof data, format, list);

    va_end(list);
    va_end(copy);
    return n->evaluate == channel_info ? channel_send(n, data, sizeof data - 1)
                                       : user_send(n, data, sizeof data - 1);
}