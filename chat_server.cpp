#include "chat_server.h"
#include "sock_util.h"

#include <iostream>

#include <thread>
#include <mutex>
#include <unordered_map>

#define DEFAULT_BUFLEN  512
#define DEFAULT_PORT    "3490"
#define MAX_USERS       3

// Global map <username -> socket id>
std::unordered_map<std::string, int> userToSocket;
int currentUsers {0};
std::mutex mtx;

void forward(std::string from, std::string to, std::string msg) {
    if (isOnlineUser(to)) {
        SOCKET dest = userToSocket[to];
        sendMsg(dest, (from + ": " + msg + "\n").c_str());
    }
    else {
        SOCKET src = userToSocket[from];
        sendMsg(src, (to + " is currently not online.\n").c_str());
    }
};

void sendMsg(SOCKET sock, const char *msg) {
    size_t msgLen {strlen(msg)};
    int status = send(sock, msg, msgLen, 0);
    if (status == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
    }
};

std::string getOnlineUsers() {
    std::string usersList {"Currently online:\n"};
    for (auto user: userToSocket) {
        usersList += (user.first + "\n");
    }
    return usersList;
};

bool isOnlineUser(std::string username) {
    return !(userToSocket.find(username) == userToSocket.end());
}

std::tuple<char, std::string, std::string> parseMsg(const char *buf, size_t len) {
    // Extract command prefix (either ! or @)
    char prefix = buf[0];
    // Convert remainder of buf to string
    std::string str(buf, len);
    // Extract command option (excl. prefix)
    std::string option = str.substr(1, str.find(' ') - 1);
    // Extract remainder of message
    std::string data = str.substr(str.find(' ') + 1);

    return {prefix, option, data};
};

void handleClient(SOCKET sock, std::string username) {
    int status;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen {DEFAULT_BUFLEN};

    // Maintain the connection until client closes it or an error occurs
    while ((status = recv(sock, recvbuf, recvbuflen, 0)) > 0) {
        auto parsed = parseMsg(recvbuf, status);
        const char *sendbuf;
        if (std::get<0>(parsed) == '@') {
            forward(username, std::get<1>(parsed), std::get<2>(parsed));
        }
        else if (std::get<0>(parsed) == '!') {
            sendbuf = getOnlineUsers().c_str();
            sendMsg(sock, sendbuf);
            // !calculcate (sum)
        }
        else {
            sendMsg(sock, "Invalid option.\n");
        }

    }

    mtx.lock();
    userToSocket.erase(username);
    currentUsers--;
    mtx.unlock();

    std::cout << username << " (" << sock << ")" << " has disconnected.\nCurrent no. users: " << currentUsers << std::endl << std::endl;
    
    sock_close(sock);

    return;
};

int main(int nargs, char **argv) {
    // Initiate use of the Winsock DLL
    int status;
    status = sock_init();
    if (status != 0) {
        printf("WSAStartup failed with error: %d\n", status);
        return 1;
    }

    // Get address information
    struct addrinfo hints, *res;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
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

    // Listen on port for incoming connections
    status = listen(master_sock, MAX_USERS);
    if (status == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        sock_close(master_sock);
        return 1;
    }

    std::cout << "Listening for incoming connections." << std::endl << std::endl;
    
    while (true) {
        int new_sock = accept(master_sock, NULL, NULL);

        // Receive login request
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen {DEFAULT_BUFLEN};
        status = recv(new_sock, recvbuf, recvbuflen, 0);
        if (status == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }
        else {
            // Parse login request and get username
            auto parsed = parseMsg(recvbuf, status);
            std::string username = std::get<2>(parsed);

            // If username exists, reject login request
            if (isOnlineUser(username)) {
                sendMsg(new_sock, "DUPLICATE-USERNAME");
                sock_close(new_sock);
                continue;
            }

            mtx.lock();
            userToSocket[username] = new_sock;
            currentUsers++;
            mtx.unlock();

            std::cout << username << " (" << new_sock << ")" << " joined.\nCurrent no. users: " << currentUsers << std::endl << std::endl;

            // Notify new client of established connection
            const char *sendbuf = ("Welcome " + username).c_str();
            sendMsg(new_sock, sendbuf);

            // Start a thread for handling the new connection
            std::thread newUserThread(handleClient, new_sock, username);
            newUserThread.detach();
        }
    }

    sock_close(master_sock);

    sock_quit();

    return 0;
}
