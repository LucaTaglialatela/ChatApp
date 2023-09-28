#include <iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <thread>
#include <string>
#include <mutex>
#include <unordered_map>

#include "sock_util.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "3490"
#define HANDSHAKE_MSG "HELLO-FROM"

std::string bufToString(const char *source, size_t len) {
    std::string str(source, len - 1);
    return str;
};

void handleClient(SOCKET sock) {
    return;
};

int main(int nargs, char **argv) {
    // int status, activity, currentUsers = 0, client_socket[3] = {0}, socketID;
    // std::string client_name[3] = {""}, inboundRequest, reply;
    // const char* sendBuf;
    // bool invalidUsername = false, validUser = false;
    int status;
    int currentUsers {0};
    std::mutex mtx;

    // Initiate use of the Winsock DLL
    status = sock_init();
    if (status != 0) {
        printf("WSAStartup failed with error: %d\n", status);
        return 1;
    }

    struct addrinfo hints, *res;

    // Get address information
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;        // IP-type (v4/v6) unspecified
    hints.ai_socktype = SOCK_STREAM;    // Use TCP stream
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;        // Auto IP selection
    status = getaddrinfo(NULL, DEFAULT_PORT, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error code: " << status << std::endl;
        return 1;
    }

    // Create socket
    SOCKET master_sock { socket(res->ai_family, res->ai_socktype, res->ai_protocol) };
    if (master_sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        sock_close(master_sock);
        return 1;
    }

    // Bind socket to the port passed in to getaddrinfo()
    status = bind(master_sock, res->ai_addr, (int)res->ai_addrlen);
    if (status == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(res);
        sock_close(master_sock);
        return 1;
    }

    freeaddrinfo(res);
    
    std::cout << "The server is running on port " << DEFAULT_PORT << std::endl;

    int maxUsers {3};
    // Listen on port for incoming connections
    status = listen(master_sock, maxUsers);
    if (status == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        sock_close(master_sock);
        return 1;
    }

    std::cout << "Listening for incoming connections." << std::endl;

    std::unordered_map<std::string, int> connections;
    
    while (true) {
        int new_sock = accept(master_sock, NULL, NULL);
        std::string username;

        // Receive login request
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen {DEFAULT_BUFLEN};
        status = recv(new_sock, recvbuf, recvbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout.write(recvbuf, status) << std::endl;
            username = bufToString(recvbuf, status).substr(strlen(HANDSHAKE_MSG) + 1);
            std::cout << username << std::endl;
            // connections[username] = new_sock;
        }

        // Send request accept message
        const char *sendbuf = ("Welcome " + username).c_str();
        size_t sendbuflen {strlen(sendbuf)};
        status = send(new_sock, sendbuf, sendbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        }

        /* recv return value:
            If no error occurs, recv returns the number of bytes received and the buffer pointed to by the buf parameter
            will contain this data received. If the connection has been gracefully closed, the return value is zero. */

        // MAKE SURE TO DETACH NEW CLIENT SOCKET CONNECTION HANDLER ETC.
        mtx.lock();
        currentUsers++;
        mtx.unlock();

        // std::thread newUserThread(handleClient, new_sock);
        // newUserThread.detach();
        std::cout << currentUsers << std::endl;
    }

    sock_close(master_sock);

    return 0;
}
