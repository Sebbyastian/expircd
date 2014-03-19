#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#define WIN32
#define WINVER 0x0501

#define HOSTNAME "misconfigured.expircd"
#define SERVICE { .bindaddr = NULL, .bindport = "6667", .type = server_accept }, \
                { .bindaddr = NULL, .bindport = "7000", .type = server_accept },

#define NICKLEN 32
#define USERLEN 32
#define HOSTLEN 256
#define TOPICLEN 384

#define CASEMAPPING rfc1459

#endif