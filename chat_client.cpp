#include <iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <thread>
#include <string>

#include "sock_util.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "3490"

int main(int nargs, char **argv) {
    const char *IP {"127.0.0.1"};
    const char *PORT {DEFAULT_PORT};
    bool loggedIn {false};
    bool isStopped {false};

    // Initiate use of the Winsock DLL
    int status;
    status = sock_init();
    if (status != 0) {
        std::cerr << "WSAStartup error code: " << status << std::endl;
        return 1;
    }

    // Get server address information
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    status = getaddrinfo("127.0.0.1", "3490", &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error code: " << status << std::endl;
        return 1;
    }
    
    // Create socket using server address information
    SOCKET sock { socket(res->ai_family, res->ai_socktype, res->ai_protocol) };
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        sock_close(sock);
        return 1;
    }

    // Connect socket to server
    status = connect(sock, res->ai_addr, (int)res->ai_addrlen);
    if (status == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        sock_close(sock);
        return 1;
    }
    
    // Free memory storing server address information
    freeaddrinfo(res);

    // Ask user for a username
    std::cout << "Please enter a username: ";
        
    std::string userName;
    std::cin >> userName;
    
    std::string loginFormat = "!login " + userName;

    // Try to log into server using chosen username
    // TODO: avoid duplicate usernames
    do {
        // Request server login
        const char *sendbuf = loginFormat.c_str();
        size_t sendbuflen {strlen(sendbuf)};
        status = send(sock, sendbuf, sendbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        }

        // Receive server response
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen {DEFAULT_BUFLEN};
        status = recv(sock, recvbuf, recvbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout.write(recvbuf, status) << std::endl;
        }
    } while (status == SOCKET_ERROR);

    // Create threads for sending and receiving data
    // std::thread thread(function, arguments);
    std::thread socketThread {std::thread(readFromSocket, sock)};
    std::thread stdinThread {std::thread(readFromStdin, sock)};
    socketThread.join();
    stdinThread.join();

    sock_close(sock);

    return 0;
};

// bool sock_valid(SOCKET socket) {
//     return socket != INVALID_SOCKET;
// };

// int sock_error_code() {
//     return WSAGetLastError();
// };

int sock_quit() {
    return WSACleanup();
};
