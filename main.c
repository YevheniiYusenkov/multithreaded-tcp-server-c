#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT "4888"
#define CLIENT_RECV_BUFLEN 1024
#define CLIENT_SEND_BUFLEN 1024
#define CLIENTS_LIMIT 20

WINBOOL shutdownServer = FALSE;

typedef struct {
    int socketId;
    SOCKET clientSocket;
} ThreadData;

SOCKET clients[CLIENTS_LIMIT] = {0};

DWORD WINAPI handleSocket(LPVOID lpParam) {
    ThreadData* threadData = (ThreadData*)lpParam;
    SOCKET clientSocket = threadData->clientSocket;

    BYTE recvbuf[CLIENT_RECV_BUFLEN] = {0};
    int recvbuflen = CLIENT_RECV_BUFLEN;

    BYTE sendbuf[CLIENT_SEND_BUFLEN] = {0};

//    const char response[] =
//            "HTTP/1.1 200 OK\r\n"
//            "Content-Type: text/html\r\n"
//            "\r\n";

    int resultLen;
    while (!shutdownServer) {
        resultLen = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (resultLen > 0) {
            printf("%s\n", recvbuf);
            printf("recieved data from client #%d\n", threadData->socketId);

            strcpy(sendbuf, "Hello from server!");
            if (send(clientSocket, sendbuf, strlen(sendbuf), 0) == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                clients[threadData->socketId] = 0;
                return EXIT_FAILURE;
            }

//            FILE* file = fopen("C:\\Users\\yevhe\\CLionProjects\\testc\\index.html", "r+");
//            if (file == NULL) {
//                printf("failed to open file.\n");
//                closesocket(clientSocket);
//                clients[threadData->socketId] = 0;
//                return EXIT_FAILURE;
//            }
//
//            send(clientSocket, response, strlen(response), 0);
//            send(clientSocket, file, 368, 0);
//
//            fclose(file);

        } else if (resultLen == 0) {
            printf("closing connection. 0\n");
            closesocket(clientSocket);
            clients[threadData->socketId] = 0;
            return EXIT_FAILURE;
        } else {
            printf("closing connection. 1\n");
            closesocket(clientSocket);
            clients[threadData->socketId] = 0;
            return EXIT_FAILURE;
        }
    }

    closesocket(clientSocket);
    clients[threadData->socketId] = 0;
    return EXIT_SUCCESS;
}

int getFreeSocketIndex() {
    for (int i = 0; i < CLIENTS_LIMIT; ++i) {
        if (clients[i] == 0) {
            return i;
        }
    }
    return -1;
}

DWORD WINAPI processNetwork() {
    char port[] = SERVER_PORT;

    int freeSocketIndex;

    WSADATA wsaData;

    HANDLE threadHandles[CLIENTS_LIMIT] = {0};
    ThreadData threadsData[CLIENTS_LIMIT] = {0};

    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if(WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        printf("WSAStartup failed\n");
        return EXIT_FAILURE;
    }

    if (getaddrinfo(NULL, port, &hints, &result)) {
        printf("getaddrinfo failed\n");
        WSACleanup();
        return EXIT_FAILURE;
    }

    SOCKET serverSocket = INVALID_SOCKET;
    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (serverSocket == INVALID_SOCKET) {
        printf("Error at socket()\n");
        freeaddrinfo(result);
        WSACleanup();
        return EXIT_FAILURE;
    }

    if (bind(serverSocket, result->ai_addr, (int)result->ai_addrlen)) {
        printf("bind failed\n");
        freeaddrinfo(result);
        closesocket(serverSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }
    freeaddrinfo(result);

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }
    printf("Listening at port %s.\n", port);

    while (!shutdownServer) {
        freeSocketIndex = getFreeSocketIndex();
        if (freeSocketIndex != -1) {
            clients[freeSocketIndex] = accept(serverSocket, NULL, NULL);
            if (clients[freeSocketIndex] == INVALID_SOCKET) {
                printf("accept failed\n");
                closesocket(serverSocket);
                WSACleanup();
                return EXIT_FAILURE;
            }

            threadsData[freeSocketIndex].socketId = freeSocketIndex;
            threadsData[freeSocketIndex].clientSocket = clients[freeSocketIndex];

            threadHandles[freeSocketIndex] = CreateThread(NULL, 0, handleSocket, &threadsData[freeSocketIndex], 0, NULL);
            if (threadHandles[freeSocketIndex] == NULL) {
                printf("Failed to create thread\n");
                closesocket(serverSocket);
                closesocket(*((SOCKET*)clients[freeSocketIndex]));
                WSACleanup();
                return EXIT_FAILURE;
            }
        }
    }

    WaitForMultipleObjects(CLIENTS_LIMIT, threadHandles, 1, INFINITE);
    for (int i = 0; i < CLIENTS_LIMIT; ++i) {
        CloseHandle(threadHandles[i]);
    }

    closesocket(serverSocket);
    WSACleanup();
    return EXIT_SUCCESS;
}

int main() {
    DWORD processId = GetCurrentProcessId();
    printf("My process ID: %lu\n", processId);

    HANDLE networkThread;

    printf("Server started.\n");

    networkThread = CreateThread(NULL, 0, processNetwork, NULL, 0, NULL);
    if (networkThread == NULL) {
        printf("Failed to create thread\n");
        return EXIT_FAILURE;
    }

    while (!shutdownServer) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            CloseHandle(networkThread);
            shutdownServer = TRUE;
        }

        Sleep(100);
    }

    printf("Server is off.\n");

    return EXIT_SUCCESS;
}