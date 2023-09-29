#include <winsock2.h>
#include <tuple>
#include <string>

void sendMsg(SOCKET sock, const char *msg);

std::string getOnlineUsers();

bool isOnlineUser(std::string username);

std::tuple<char, std::string, std::string> parseMsg(const char *buf, size_t len);

void handleClient(SOCKET sock, std::string username);

