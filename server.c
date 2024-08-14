#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#endif

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define MAX_MESSAGE_SIZE 1024

#ifdef _WIN32
// Thread function prototype for Windows
void chat_with_client(void *client_socket_ptr) {
    SOCKET client_socket = *(SOCKET *)client_socket_ptr;
#else
// Thread function prototype for Unix-like systems
void *chat_with_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;
#endif
    free(client_socket_ptr); // Free allocated memory for client socket pointer
    char client_message[MAX_MESSAGE_SIZE];
    char server_message[MAX_MESSAGE_SIZE];
    int n;

    while (1) {
        // Clear message buffers
        memset(client_message, 0, sizeof(client_message));
        // Receive message from client
        n = recv(client_socket, client_message, sizeof(client_message), 0);
        if (n < 0) {
            perror("Error reading from socket");
            break;
        } else if (n == 0) {
            printf("Client disconnected.\n");
            break;
        }
        printf("Received from client: %s\n", client_message);

        // Prompt for server response
        printf("Enter response to client: ");
        // Get server message from stdin
        fgets(server_message, sizeof(server_message), stdin);

        // Send response to client
        n = send(client_socket, server_message, strlen(server_message), 0);
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        printf("Sent to client: %s\n", server_message);
    }

    // Close client socket and exit thread
#ifdef _WIN32
    closesocket(client_socket);
    ExitThread(0);
#else
    close(client_socket);
    pthread_exit(NULL);
#endif
}

int main() {
    #ifdef _WIN32
    WSADATA wsa_data;
    SOCKET server_fd;
    #else
    int server_fd;
    #endif
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    // Initialize Winsock (Windows only)
    #ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        perror("WSAStartup failed");
        return 1;
    }
    #endif

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed");
        return 1;
    }

    // Setup server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Bind failed");
        #ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
        #else
        close(server_fd);
        #endif
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_PENDING_CONNECTIONS) == SOCKET_ERROR) {
        perror("Listen failed");
        #ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
        #else
        close(server_fd);
        #endif
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept incoming connections and handle them in separate threads
    while (1) {
        int *client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) == INVALID_SOCKET) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        printf("Client connected.\n");

        // Create a new thread to handle client communication
        #ifdef _WIN32
        HANDLE thread_handle;
        thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)chat_with_client, (void *)client_socket, 0, NULL);
        if (thread_handle == NULL) {
            perror("Failed to create thread");
            free(client_socket);
            continue;
        }
        CloseHandle(thread_handle);  // Close the handle since it's not needed
        #else
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, chat_with_client, (void *)client_socket) != 0) {
            perror("Failed to create thread");
            free(client_socket);
            continue;
        }
        pthread_detach(thread_id);  // Ensure resources are released after thread finishes
        #endif
    }

    // Cleanup and close server socket (platform-specific)
    #ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
    #else
    close(server_fd);
    #endif

    return 0;
}
