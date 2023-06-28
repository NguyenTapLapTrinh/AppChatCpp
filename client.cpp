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

DWORD WINAPI SendThread(LPVOID lpParam)
{
    SOCKET ConnectSocket = (SOCKET)lpParam;
    char sendbuf[DEFAULT_BUFLEN];

    while (1) {
        fgets(sendbuf, sizeof(sendbuf), stdin);

        // Remove newline character from the input
        sendbuf[strcspn(sendbuf, "\n")] = '\0';

        // Check if user wants to quit
        if (strcmp(sendbuf, "q") == 0)
            break;

        // Send the message to the server
        int iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            return 1;
        }

        //printf("Message sent to server\n");
    }

    // Close the socket
    closesocket(ConnectSocket);
    return 0;
}

DWORD WINAPI ReceiveThread(LPVOID lpParam)
{
    SOCKET ConnectSocket = (SOCKET)lpParam;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Server message: %.*s\n", iResult, recvbuf);
        }
        else if (iResult == 0)
            printf("Server connection closed\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            return 1;
        }

    } while (iResult > 0);

    // Close the socket
    closesocket(ConnectSocket);
    return 0;
}

int main(void) 
{
    WSADATA wsaData;
    int iResult;
    SOCKET ConnectSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to the server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to the server
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Connected to the server\n");

    // Create a thread to handle sending messages
    HANDLE hSendThread = CreateThread(NULL, 0, SendThread, (LPVOID)ConnectSocket, 0, NULL);
    if (hSendThread == NULL) {
        printf("CreateThread failed with error: %d\n", GetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Create a thread to handle receiving messages from server
    HANDLE hReceiveThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)ConnectSocket, 0, NULL);
    if (hReceiveThread == NULL) {
        printf("CreateThread failed with error: %d\n", GetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Wait for both threads to complete
    WaitForSingleObject(hSendThread, INFINITE);
    WaitForSingleObject(hReceiveThread, INFINITE);

    // Close the thread handles
    CloseHandle(hSendThread);
    CloseHandle(hReceiveThread);

    // Cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}