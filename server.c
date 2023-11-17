#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3560"
#define MAXBUFFERSIZE 100
#define MAXCONNECTIONS 10

void sigchld_handler(){
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr* sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int create_and_bind_socket(){
    int sockfd, rv;
    struct addrinfo *p, *servinfo, hints;
    char s[INET6_ADDRSTRLEN]; 
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo) == -1)){
        fprintf(stderr, "Issue in GetAddrInfo %s\n", gai_strerror(rv));
        exit(-1);
    }
    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("server:socket");
            continue;
        }
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1){
            perror("setsockopt");
            exit(1);
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            perror("server:bind");
            continue;
        }
        break;
    }
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
    printf("Client Connected to Address: %s \n", s);
    free(servinfo);
    return p == NULL ? -1 : sockfd; 
}

void server_loop(int* sockfd){
    int new_sockfd, n_read, n_sent; 
    int our_sockfd = *sockfd;
    char buffer[MAXBUFFERSIZE];
    socklen_t sock_size;
    struct sockaddr_storage their_addr;
    printf("In Server Loop!\n");
    while(1){
        sock_size = sizeof their_addr;
        new_sockfd = accept(our_sockfd, (struct sockaddr *)&their_addr, &sock_size);
        if(new_sockfd == -1){
            perror("accept");
            continue;
        }
        if(!fork()){
            close(our_sockfd);
            n_read = recv(new_sockfd, &buffer, MAXBUFFERSIZE, 0);
            if(n_read >= 0){
                buffer[n_read + 1] = '\0';
                printf("Read %s from client\n", buffer);
            }
            n_sent = send(new_sockfd, &buffer, MAXBUFFERSIZE, 0);
            if(n_sent >= 0){
                printf("Sent %s to client\n", buffer);
            }
            close(new_sockfd);
            exit(0);
        }
    }
}


int main(void){
    int sockfd;
    struct sigaction sa;
    if((sockfd = create_and_bind_socket()) == -1){
        fprintf(stderr, "Unable to bind to any sockets");
        exit(1);
    } 
    if(listen(sockfd, MAXCONNECTIONS) < 0){
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    server_loop(&sockfd);
}

