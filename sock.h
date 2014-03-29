#ifndef INCLUDE_SOCK_H
#define INCLUDE_SOCK_H

#ifdef CONFIG
#    include CONFIG
#endif

#ifdef WIN32
#    define errno            WSAGetLastError()
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    define accept(fd)       (accept(fd, NULL, NULL))
#    define sock_again(fd)   (WSAGetLastError() == WSAEWOULDBLOCK)
#    define sock_invalid(fd) (fd == INVALID_SOCKET)
#    define listen(fd, addr) (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0 && addr->ai_socktype == SOCK_DGRAM || listen(fd, 16) == 0)
#    define set_nonblock(fd) (ioctlsocket(fd, FIONBIO, (u_long[]){1}) == 0)
typedef SOCKET sockfd;
#else
#    include <netdb.h>
#    include <fcntl.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    define closesocket(fd)  close(fd)
#    define accept(fd)       (accept(fd, NULL, (socklen_t[]) { 0 }))
#    define sock_again(fd)   (errno == EAGAIN || errno == EWOULDBLOCK)
#    define sock_invalid(fd) (fd < 0)
#    define listen(fd, addr) (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0 && addr->ai_socktype == SOCK_DGRAM || listen(fd, 16) == 0)
#    define set_nonblock(fd) (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) != -1)
typedef int sockfd;
#endif

typedef struct addrinfo addrinfo;
#endif
