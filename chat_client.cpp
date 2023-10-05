#include "chat_client.h"
#include "sock_util.h"

#include <iostream>
#include <algorithm>

#include <thread>
#include <string>

#define DEFAULT_BUFLEN  512
#define DEFAULT_IP      "127.0.0.1"
#define DEFAULT_PORT    "3490"

std::string createUsername() {
    std::string userName;
        
    std::cout << "Please enter a username: ";
    std::cin >> userName;

    return "!login " + userName;
};

void readFromSocket(SOCKET sock) {
    int status;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen { DEFAULT_BUFLEN };

    do {
        status = recv(sock, recvbuf, recvbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
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

int main(int nargs, char **argv) {
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
    status = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &res);
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

    // Create a login request
    const char *loginRequest = createUsername().c_str();
    bool loginSuccess = false;
    
    do {
        // Request server login
        size_t loginRequestLen {strlen(loginRequest)};
        status = send(sock, loginRequest, loginRequestLen, 0);
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
            // Handle server response
            if (std::equal(recvbuf, recvbuf + status, "DUPLICATE-USERNAME")) {
                std::cout << "That username already exists. Try again." << std::endl;
                break;
            }
            // If other case
            //      do y
            else {
                loginSuccess = true;
                std::cout.write(recvbuf, status) << std::endl;
            }
        }
    } while (!loginSuccess);

    // Create threads for sending and receiving data
    // std::thread thread(function, arguments);
    if (loginSuccess) {
        std::thread socketThread {std::thread(readFromSocket, sock)};
        std::thread stdinThread {std::thread(readFromStdin, sock)};
        socketThread.join();
        stdinThread.join();
    }

    sock_close(sock);

    sock_quit();

    return 0;
};
