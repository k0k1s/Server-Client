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

void *doReceiving(void *sockID) {

    int clientSocket = *((int *) sockID);
    char username[1024];

    // Receive username from server
    int bytesReceived = recv(clientSocket, username, 1024, 0);
    if (bytesReceived <= 0) {
        printf("Connection closed by server\n");
        close(clientSocket);
        exit(EXIT_SUCCESS);
    }
    username[bytesReceived] = '\0';
    printf("You are connected as %s\n", username);

    while (1) {
        char data[2048];
        bytesReceived = recv(clientSocket, data, 2048, 0);
        if (bytesReceived <= 0) {
            printf("Connection closed by server\n");
            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
        data[bytesReceived] = '\0';
        printf("%s\n", data);
    }
}

int main() {

    int clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6001); // Port 6001 for waiting room
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Connect to localhost

    if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    char username[1024];
    printf("Enter your username: ");
    scanf("%s", username);
    send(clientSocket, username, strlen(username), 0);

    printf("Waiting Room connected ............\n");
    printf("Instructions:\n");
    printf("- To enter the chat room, type 'CHAT' and press enter.\n");

    while (1) {
        char input[2048];
        scanf(" %[^\n]", input); // Read the entire line

        if (strcmp(input, "CHAT") == 0) {
            send(clientSocket, input, strlen(input), 0);
            break;
        } else {
            printf("Invalid command. Please type 'CHAT' to enter the chat room.\n");
        }
    }

    pthread_t thread;
    pthread_create(&thread, NULL, doReceiving, (void *) &clientSocket);

    printf("Connected to Chat Room ............\n");
    printf("Instructions:\n");
    printf("- To send a broadcast message, simply type your message and press enter.\n");
    printf("- To send an individual message, type the recipient's username followed by '/'.\n");
    printf("  Then type your message.\n");

    while (1) {
        char input[2048];
        scanf(" %[^\n]", input); // Read the entire line

        char sendMessage[2048];
        sprintf(sendMessage, "%s", input);
        send(clientSocket, sendMessage, strlen(sendMessage), 0);

        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
    }

    close(clientSocket);
    pthread_cancel(thread);

    return 0;
}
