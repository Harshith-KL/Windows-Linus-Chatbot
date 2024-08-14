#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT 8080
#define MAX_MESSAGE_SIZE 1024

int main() {
    #ifdef _WIN32
    WSADATA wsa_data;
    SOCKET sock = 0;
    #else
    int sock = 0;
    #endif
    struct sockaddr_in server_addr;
    char message[MAX_MESSAGE_SIZE] = {0};
    char buffer[MAX_MESSAGE_SIZE] = {0};

    // Initialize Winsock (Windows only)
    #ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        perror("WSAStartup failed");
        return 1;
    }
    #endif

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation error");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    // Server address setup
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
        #else
        close(sock);
        #endif
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
        #else
        close(sock);
        #endif
        return 1;
    }

    printf("Connected to server.\n");

    // Communication loop
    while (1) {
        printf("Enter message (type 'exit' to close): ");
        fgets(message, sizeof(message), stdin);

        // Send message to server
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            break;
        }

        // Clear buffer and receive response from server
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        } else if (bytes_received == 0) {
            printf("Server disconnected.\n");
            break;
        }

        printf("Server response: %s\n", buffer);

        // Exit condition
        if (strcmp(message, "exit\n") == 0)
            break;
    }

    // Cleanup and close socket (platform-specific)
    #ifdef _WIN32
    closesocket(sock);
    WSACleanup();
    #else
    close(sock);
    #endif

    return 0;
}
