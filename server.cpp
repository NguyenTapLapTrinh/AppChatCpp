#undef UNICODE
#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
int shouldRespond = 0;
DWORD WINAPI HandleClient(LPVOID lpParam)
{
    SOCKET clientSocket = (SOCKET)lpParam;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;
    do {
    iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
        printf("Client message: %.*s\n", iResult, recvbuf);

        // Kiểm tra giá trị của biến flag
        if (shouldRespond == 0) {
            // Echo the received message back to the client
            iResult = send(clientSocket, recvbuf, iResult, 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                return 1;
            }

            // Đặt biến flag thành "phản hồi"
            shouldRespond = 1;
        }
    }
    else if (iResult == 0)
        printf("Client connection closed\n");
    else {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        return 1;
    }
    } while (iResult > 0);

    // Cleanup
    closesocket(clientSocket);
    return 0;
}

DWORD WINAPI SendThread(LPVOID lpParam)
{
    SOCKET clientSocket = (SOCKET)lpParam;
    char sendbuf[DEFAULT_BUFLEN];

    while (1) {
        fgets(sendbuf, sizeof(sendbuf), stdin);

        // Remove newline character from the input
        sendbuf[strcspn(sendbuf, "\n")] = '\0';

        // Check if user wants to quit
        if (strcmp(sendbuf, "q") == 0)
            break;

        // Send the message to the client
        int iResult = send(clientSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            return 1;
        }

        //printf("Message sent to client\n");
    }

    // Close the socket
    closesocket(clientSocket);
    return 0;
}

int main(void) 
{
    WSADATA wsaData;
    int iResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the listening socket to the specified IP address and port
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Start listening for client connections
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening for connections...\n");

    while (1) {
        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        printf("Client connected\n");

        // Create a new thread to handle the client connection
        HANDLE hThread = CreateThread(NULL, 0, HandleClient, (LPVOID)ClientSocket, 0, NULL);
        if (hThread == NULL) {
            printf("CreateThread failed with error: %d\n", GetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

        // Close the thread handle to allow it to run independently
        CloseHandle(hThread);

        // Create a thread to handle sending messages to the client
        HANDLE hSendThread = CreateThread(NULL, 0, SendThread, (LPVOID)ClientSocket, 0, NULL);
        if (hSendThread == NULL) {
            printf("CreateThread failed with error: %d\n", GetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

        // Close the thread handle to allow it to run independently
        CloseHandle(hSendThread);
    }

    // Cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}