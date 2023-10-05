#include "sock_util.h"

#include <iostream>
#include <string>

#define DEFAULT_BUFLEN 512

int sock_init() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
};

int sock_quit() {
    return WSACleanup();
}

int sock_close(SOCKET sock) {
    shutdown(sock, SD_BOTH);
    return closesocket(sock);
};
