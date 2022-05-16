#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>

#define MAX_CLIENT 5
#define PORT 8034

void *handlerFunction(void *);

int main(int argc, char *argv[])
{
    int serverSocket, clientSocket, c, *newSocket;
    struct sockaddr_in servAddr, clientAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
    {
        printf("\nSocket not created!!!");
    }
    puts("\nSocket created");

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

    if(bind(serverSocket,(struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("\nError bind!!!");
        return 1;
    }
    puts("\nBind OK!!");

    listen(serverSocket, MAX_CLIENT);

    puts("Server waiting clients");

    c = sizeof(struct sockaddr_in);
    while((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t*)&c)) )
    {
        puts("Connection Accepted");

        pthread_t connectThread;
        newSocket = malloc(1);
        *newSocket = clientSocket;

        if(pthread_create(&connectThread, NULL, handlerFunction, (void*) newSocket) < 0)
        {
            perror("Error created thread!!!");
            return 1;
        }
        pthread_join(connectThread, NULL);
        puts("Handler function assigned!!!");
    }

    if(clientSocket < 0)
    {
        perror("Error accept");
        return 1;
    }

    return 0;

}

void *handlerFunction(void *serverSock)
{
    int sock = *(int*)serverSock;
    int readSize;
    char *buffer, clientBuffer[2000];
    buffer = "\nHello!!!!!";
    write(sock, buffer, strlen(buffer));
    buffer = "\nHave a good day!!!";
    write(sock, buffer, strlen(buffer));

    while((readSize = recv(sock, clientBuffer, 2000, 0)) > 0)
    {
        puts(clientBuffer);
        fflush(stdout);
        write(sock, clientBuffer, strlen(clientBuffer));
    }

    if(readSize == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(readSize == -1)
    {
        perror("Error recv!!!");
    }

    free(serverSock);

    return 0;

}
