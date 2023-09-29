#include <iostream>
// #include <winsock2.h>
#include <Ws2tcpip.h>

#include <thread>
#include <mutex>
#include <unordered_map>

#include "sock_util.h"
#include "chat_server.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "3490"

// Global map <username -> socket id>
std::unordered_map<std::string, int> userToSocket;
int currentUsers {0};
std::mutex mtx;

enum Options { login, get_users };
std::unordered_map<std::string, Options> getOption {
    {"login", login},
    {"users", get_users}
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
        usersList += (user.first + ": " + std::to_string(user.second) + "\n");
    }
    return usersList;
};

bool isOnlineUser(std::string username) {
    return !(userToSocket.find(username) == userToSocket.end());
}

/* User commands:
**  !login: Sent on login by new user
**  !users: Display list of online users.
**  !quit: Quit the application (client only).
**  @username msg: Send a message to user with username username.
*/

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
    while (status = recv(sock, recvbuf, recvbuflen, 0) > 0) {
        auto parsed = parseMsg(recvbuf, status);
        const char *sendbuf;
        if (std::get<0>(parsed) == '@') {
            std::string recipient {std::get<1>(parsed)};
            if (isOnlineUser(recipient)) {
                SOCKET dest = userToSocket[recipient];
                sendbuf = (username + ": " + std::get<2>(parsed) + "\n").c_str();
                sendMsg(dest, sendbuf);
            }
            else {
                sendbuf = (recipient + " is currently not online.\n").c_str();
                sendMsg(sock, sendbuf);
            }
            
        }
        else if (std::get<0>(parsed) == '!') {
            sendbuf = getOnlineUsers().c_str();
            sendMsg(sock, sendbuf);
        }
        else {
            sendbuf = "Invalid option.\n";
            sendMsg(sock, sendbuf);
        }

    }

    mtx.lock();
    userToSocket.erase(username);
    currentUsers--;
    mtx.unlock();

    return;
};

int main(int nargs, char **argv) {
    // int status, activity, currentUsers = 0, client_socket[3] = {0}, socketID;
    // std::string client_name[3] = {""}, inboundRequest, reply;
    // const char* sendBuf;
    // bool invalidUsername = false, validUser = false;
    int status;

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

    std::cout << "Listening for incoming connections." << std::endl << std::endl;
    
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
            // std::cout.write(recvbuf, status) << std::endl;
            auto parsed = parseMsg(recvbuf, status);
            username = std::get<2>(parsed);
        }

        /* recv return value:
            If no error occurs, recv returns the number of bytes received and the buffer pointed to by the buf parameter
            will contain this data received. If the connection has been gracefully closed, the return value is zero. */

        mtx.lock();
        userToSocket[username] = new_sock;
        currentUsers++;
        mtx.unlock();

        std::cout << username << " (" << new_sock << ")" << " joined.\nCurrent no. users: " << currentUsers << std::endl << std::endl;

        // Send login success message
        const char *sendbuf = ("Welcome " + username).c_str();
        sendMsg(new_sock, sendbuf);

        std::thread newUserThread(handleClient, new_sock, username);
        newUserThread.detach();
    }

    sock_close(master_sock);

    return 0;
}
