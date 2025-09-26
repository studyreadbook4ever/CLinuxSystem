#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message) { fputs(message, stderr); fputc('\n', stderr); exit(1); }
//this code does not open suspend - just for understanding system call
int main () {
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) error_handling("socket() error");
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //ipv4 address - INET6 is ipv6
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //0.0.0.0 (all)
    serv_addr.sin_port = htons(9999);

    int option = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); //no port number race condition
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) { error_handling("bind() error"); }
    printf("socket address giving complete!\n");
    printf("now port is opened on 9999.\n");
    close(serv_sock);
    return 0;
}
