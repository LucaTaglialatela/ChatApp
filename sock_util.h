#ifndef SOCK_UTIL_H
#define SOCK_UTIL_H

#include <winsock2.h>
#include <Ws2tcpip.h>

int sock_init();

int sock_quit();

int sock_close(SOCKET sock);

#endif
