#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include "sock_util.h"

#include <tuple>
#include <string>

// Forwards a message `msg` with source `from` and destination `to`
void forward(std::string from, std::string to, std::string msg);

// Sends message `msg` over socket `sock`
void sendMsg(SOCKET sock, const char *msg);

// Returns formatted list of online users
std::string getOnlineUsers();

// Returns whether user `username` is online
bool isOnlineUser(std::string username);

// Parses the result of recv so it can be resolved by the server
std::tuple<char, std::string, std::string> parseMsg(const char *buf, size_t len);

// Maintains the connection with a client and resolves their requests
void handleClient(SOCKET sock, std::string username);

#endif
