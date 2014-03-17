#include "node.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct service {
    char *bindaddr;
    char *bindport;
    evaluator *type;
} service;

int main(void) {
    addrinfo *addr;
    nodeinfo *node = NULL;
    service service[] = { SERVICE };

#   ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
        fputs("FATAL: WSAStartup failed.", stderr);
        return 0;
    }
#   endif

    for (size_t x = 0; x < sizeof service / sizeof *service; x++) {
        int n = getaddrinfo(service[x].bindaddr, service[x].bindport, &(addrinfo){ .ai_socktype = SOCK_STREAM,
                                                                                   .ai_family = AF_UNSPEC,
                                                                                   .ai_flags = AI_PASSIVE }, &addr);
        assert(n == 0);
        nodeinfo_bind(&node, addr, service[x].type);
        freeaddrinfo(addr);
    }

    if (node == NULL) {
        fputs("FATAL: All port bindings failed.", stderr);
        return 0;
    }

    time_t start = time(NULL);
    size_t x = 0;
    for (;;) {
        node->node[x].evaluate(node->node + x, &node);

        x++;
        if (x == node->size) {
            unsigned int y = 0;
            while (time(NULL) - start < 1) {
                usleep(100000);
                y++;
            }
            printf("Checked %zu sockets and slept for %u.%u seconds\n", x, y / 10, y % 10);
            start++;
            x = 0;
        }
    }

#   ifdef WIN32
    WSACleanup();
#   endif
    return 0;
}