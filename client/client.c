#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

SOCKET initClient() {
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup() failed: %ld\n", WSAGetLastError());
    }
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(HOST, PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET clientSocket = INVALID_SOCKET;
    ptr = result;
    clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(clientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);
    if (clientSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    printf("[*] Connected to server\n");

    return clientSocket;
}

void sendMessage(SOCKET client, char* message) {
    char buffer[DEFAULT_BUFFER];
    ZeroMemory(buffer, sizeof(buffer));
    strncpy(buffer, message, sizeof(buffer) - 1); // Copy message to buffer

    int res = send(client, buffer, strlen(buffer), 0);
    if (res == SOCKET_ERROR) {
        printf("send() failed: %ld\n", WSAGetLastError());
    }
}

char* receiveMessage(SOCKET client) {
    char buffer[DEFAULT_BUFFER];
    int res = recv(client, buffer, sizeof(buffer) - 1, 0);

    if (res > 0) {
        buffer[res] = '\0'; // Null-terminate received data
        printf("Bytes received: %d\n", res);
        printf("Received: %s", buffer);
    } 
    else if (res == 0) {
        printf("Connection closed\n");
    } 
    else {
        printf("recv() failed: %ld\n", WSAGetLastError());
    }
    
    
}