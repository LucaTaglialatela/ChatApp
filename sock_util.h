#include <winsock2.h>

int sock_init();

int sock_quit();

int sock_close(SOCKET sock);

void readFromSocket(SOCKET sock);

void readFromStdin(SOCKET sock);
