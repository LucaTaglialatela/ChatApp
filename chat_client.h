#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include "sock_util.h"

#include <string>

// Creates and formats a new username for a login request
std::string createUsername();

void readFromSocket(SOCKET sock);

void readFromStdin(SOCKET sock);

#endif
