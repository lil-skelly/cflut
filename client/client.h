#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define HOST "pixelflut.uwu.industries"
#define PORT "1234"
#define DEFAULT_BUFFER 512

SOCKET initClient();
void sendMessage(SOCKET client, char* message);
char* receiveMessage(SOCKET client);

#endif