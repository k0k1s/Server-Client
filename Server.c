#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_CLIENTS 10
#define PORT 6001

struct client {
    int clientSocket;
    char username[1024];
    char password[1024];
};

struct client clients[MAX_CLIENTS];
int clientCount = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleClient(void *clientSocket) {
    int socket = *((int *) clientSocket);
    char username[1024];
    char password[1024];
    recv(socket, username, 1024, 0);
    recv(socket, password, 1024, 0);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].clientSocket == 0) {
            clients[i].clientSocket = socket;
            strcpy(clients[i].username, username);
            strcpy(clients[i].password, password);
            clientCount++;
            break;
        }
    }

    while (1) {
        char message[2048];
        int bytesReceived = recv(socket, message, 2048, 0);
        if (bytesReceived <= 0) {
            close(socket);
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].clientSocket == socket) {
                    clients[i].clientSocket = 0;
                    clientCount--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        message[bytesReceived] = '\0';

        pthread_mutex_lock(&mutex);
        // Check if the message is for individual client
        char *recipient = strtok(message, "/");
        char *msg = strtok(NULL, "");
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(clients[i].username, recipient) == 0) {
                char sendMessage[2048];
                sprintf(sendMessage, "%s >> %s", username, msg);
                send(clients[i].clientSocket, sendMessage, strlen(sendMessage), 0);
                break;
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {

    signal(SIGCHLD, sigchld_handler);

    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections ............\n");

    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            perror("accept");
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handleClient, (void *) &clientSocket);
    }

    return 0;
}
