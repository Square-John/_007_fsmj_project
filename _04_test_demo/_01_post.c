//创建一个post请求客户端

// 步骤 1: 包含必要的头文件
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>



// 步骤 2: 初始化Winsock
int initWinsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

// 步骤 3: 创建socket并连接到服务器
SOCKET createAndConnect(const char* host, const char* port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Error at socket(): %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(port));
    inet_pton(AF_INET, host, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

// 步骤 4: 发送POST请求
void sendPostRequest(SOCKET sock, const char* host, const char* path, const char* postData) {
    char request[1024];
    sprintf(request, "POST %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Content-Length: %lld\r\n"
                 "\r\n"
                 "%s",
            path, host, strlen(postData), postData);

    if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
    }
}

// 步骤 5: 接收并打印响应
void receiveResponse(SOCKET sock) {
    char buffer[4096];
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        printf("%s", buffer);
    }
}

// 步骤 6: 清理工作
void cleanup(SOCKET sock) {
    closesocket(sock);
    WSACleanup();
}

// 步骤 7: 主函数
int main() {
    if (initWinsock() != 0) {
        printf("初始化 Winsock 失败\n");
        return -1;
    }

    SOCKET sock = createAndConnect("example.com", "80");
    if (sock == INVALID_SOCKET) {
        return -1;
    }

    const char* postData = "key=value";
    sendPostRequest(sock, "example.com", "/", postData);
    receiveResponse(sock);

    cleanup(sock);
    return 0;
}