#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "8034"  


// Get sockaddr:
void *get_address(struct sockaddr *socketAddr)
{
    if (socketAddr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socketAddr)->sin_addr);
    }

    return &(((struct sockaddr_in6*)socketAddr)->sin6_addr);
}

// Return socket
int get_socket(void)
{
    int listener;     // socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     //return all ipv4 and ipv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;       //return template addr INADDR_ANY or IN6ADDR_ANY_INIT
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) //return struct addrinfo
    {     
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) 
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) 
        { 
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }

    freeaddrinfo(ai); // All done with this

    // Error bind
    if (p == NULL) {                
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;  //Error listen
    }
    return listener; //listen ok
}

// Add a new fd to the set                        
void addToClientList(struct pollfd *clientList[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size)
    { 
        //if need more connection
        *fd_size *= 2; // Double it
        *clientList = realloc(*clientList, sizeof(**clientList) * (*fd_size)); 
    }

    (*clientList)[*fd_count].fd = newfd;
    (*clientList)[*fd_count].events = POLLIN; // Check ready-to-read
    (*fd_count)++;
}

void dellFromClientList(struct pollfd clientList[], int i, int *fd_count)
{
    clientList[i] = clientList[*fd_count-1];
    (*fd_count)--;
}

int checkBlock(const char *str)
{
    char *result;
    result = strstr(str, "BLOCKME");
    if(result == NULL )
        return 0;
    else
        return 1;
}

int checkClose(const char *str)
{
    char *result;
    result = strstr(str, "CLOSEME");
    if(result == NULL )
        return 0;
    else
        return 1;
}

int main(void)
{
    int servSocket;     

    int newClientSocket;        
    struct sockaddr_storage clientAddr; 
    socklen_t addrlen;

    char clientBuff[256];    
    char remoteIP[INET6_ADDRSTRLEN];

    int clientCount = 0;
    int clientMax = 5;
    struct pollfd *clientList = malloc(sizeof *clientList * clientMax);

    servSocket = get_socket();

    if (servSocket == -1) {
        fprintf(stderr, "error created socket\n");
        exit(1);
    }

    // Add the listener to set
    clientList[0].fd = servSocket;
    clientList[0].events = POLLIN; 

    clientCount = 1; 

    for(;;) 
    {
        int poll_count = poll(clientList, clientCount, -1);
        if (poll_count == -1) 
        {
            perror("poll");
            exit(1);
        }

        // find data
        for(int clientIndex = 0; clientIndex < clientCount; clientIndex++) {

            // Check if someone's ready to read
            if (clientList[clientIndex].revents & POLLIN) 
            { 
                if (clientList[clientIndex].fd == servSocket) 
                {
                    addrlen = sizeof clientAddr;
                    newClientSocket = accept(servSocket, (struct sockaddr *)&clientAddr, &addrlen);
                    if (newClientSocket == -1) 
                    {
                        perror(" Error accept! ");
                    } 
                    else 
                    {
                        addToClientList(&clientList, newClientSocket, &clientCount, &clientMax);
                        printf("Server: New client %d connected \n", newClientSocket);
                    }
                } 
                else 
                {
                    int nbytes = recv(clientList[clientIndex].fd, clientBuff, sizeof clientBuff, 0);
                    int clientSend = clientList[clientIndex].fd;

                    if (nbytes <= 0) 
                    {
                        if (nbytes == 0) 
                        {
                            printf("Server: client %d disconnect\n", clientSend);
                        } 
                        else 
                        {
                            perror("Error recv");
                        }
                        close(clientList[clientIndex].fd); 
                        dellFromClientList(clientList, clientIndex, &clientCount);
                    } 
                    else //if data
                    {
                            //bonus
                       if(!checkBlock(clientBuff))
                       {
                           if(!checkClose(clientBuff))
                           {
                                for(int j = 0; j < clientCount; j++) 
                                {
                                    int destClient = clientList[j].fd;
                                    if (destClient != servSocket && destClient != clientSend) 
                                    {
                                        if (send(destClient, clientBuff, nbytes, 0) == -1) 
                                        {
                                            perror("Error send");
                                        }
                                    }   
                                }
                            }
                            else
                            {
                                for(int j = 0; j < clientCount; j++) 
                                {
                                    if(clientList[j].fd == clientSend)
                                    {
                                        close(clientList[j].fd); 
                                        dellFromClientList(clientList, j, &clientCount);
                                        printf("Server: client %d disconnect\n", clientSend);
                                        break;
                                   }
                                }
                                
                            }   
                        }
                    } 
                } 
            } 
        }     
    
    }
return 0;
}