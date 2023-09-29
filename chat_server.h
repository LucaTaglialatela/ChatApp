#include <winsock2.h>
#include <tuple>
#include <string>

void sendMsg(SOCKET sock, const char *msg);

std::tuple<char, std::string, std::string> parseMsg(const char *buf, size_t len);

std::string getUsers();

bool isOnlineUser(std::string username);

void handleClient(SOCKET sock, std::string username);

