#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3560" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void *get_in_addr(struct sockaddr* sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(void){
    int sockfd, numbytes;
    int numbytes_read;
    char s[INET6_ADDRSTRLEN];
    char result_buffer[MAXDATASIZE];
    char input_buffer[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
        fprintf(stderr, "getAddressInfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0){
            perror("stream:socket");
            continue;
        }
        if((connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1){
            perror("stream:connect");
            continue;
        }
        break;
    }

    if(p == NULL){
        fprintf(stderr, "Client Failed to Connect!\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
    printf("Client Connected to Address: %s\n", s);
    printf("Input Text:");
    if(!fgets(input_buffer, MAXDATASIZE, stdin)){
        fprintf(stderr, "Issue Reading\n");
        exit(1);
    }
    printf("Read %s from input\n", input_buffer);
    freeaddrinfo(servinfo);
    if((numbytes = send(sockfd, input_buffer, strlen(input_buffer), 0)) < 0){
        perror("send");
        exit(1);
    }
    printf("Sent %d bytes to Server\n", numbytes);
    while(1){
        printf("Waiting for response.\n");
        if((numbytes_read = recv(sockfd, result_buffer, MAXDATASIZE, 0)) == -1){
            perror("recv");
            exit(1);
        }
        result_buffer[numbytes_read] = '\0';
        printf("Client Received: %s\n", result_buffer);
        close(sockfd);
        break;
    }
    return 0;
}




