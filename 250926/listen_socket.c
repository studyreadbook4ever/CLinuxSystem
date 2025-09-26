#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
void error_handling(char *message) { fputs(message, stderr); fputc('\n', stderr); exit(1); }

int main() {
    int serv_sock;
    struct sockaddr_in serv_addr;
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) error_handling("socket() error");
    int option = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&serv_addr, 0 , sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(9999);
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind() error");
    if (listen(serv_sock, 5) == -1) { error_handling("listen() error"); }
    printf("server started.. waiting for client...\n");
    printf("do 'ss -lntp | grep 9999' on shell after start it.\n"); //show listen(type a for show all), numeric, tcp, show with process info
    sleep(50);
    close(serv_sock);
    return 0;
}
