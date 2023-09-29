#include "sock_util.h"
#include <iostream>
#include <string>

#define DEFAULT_BUFLEN 512

int sock_init() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
};

int sock_close(SOCKET sock) {
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    return WSACleanup();

    // int status { shutdown(sock, SD_BOTH) };
    // if (status == 0) { status = closesocket(sock); }
    // return status;
};

void readFromSocket(SOCKET sock) {
    int status;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen { DEFAULT_BUFLEN };

    do {
        status = recv(sock, recvbuf, recvbuflen, 0);
        if (status == SOCKET_ERROR) {
            // std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout.write(recvbuf, status) << std::endl;
        }
    } while (true);
};

void readFromStdin(SOCKET sock) {
    int status;
    std::string msg;

    do {
        std::getline(std::cin, msg);
        
        if (msg == "!quit") {
            std::cout << "Shutting down." << std::endl;
            sock_close(sock);
            return;
        }

        const char *sendbuf = msg.c_str() + '\0';
        size_t sendbuflen { strlen(sendbuf) };
        status = send(sock, sendbuf, sendbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        }
    } while (true);
};
