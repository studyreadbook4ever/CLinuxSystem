//use netcat for easily connect to this server by multiprocessing with localhost

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message) { fputs(message, stderr); fputc('\n', stderr); exit(1); }

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char message[] = "Hello from server!";
    char buffer[1024];
    int str_len;
    if (argc != 2) {
        printf("usage: %s <port>\n", argv[0]);
        exit(1);
    }
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) error_handling("socket() error");
    int option = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind() error");
    if (listen(serv_sock, 5) == -1) error_handling("listen() error");
    printf("server start... port %s is waiting client...\n", argv[1]);
    clnt_addr_size = sizeof(clnt_addr);
    //wait '1' client forever while no signal....
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) error_handling("accpet() error");
    printf("client accept: IP %s\n", inet_ntoa(clnt_addr.sin_addr));
    while((str_len = read(clnt_sock, buffer, 1024)) != 0) {
        write(clnt_sock, buffer, str_len); // read msg and write again
    }
    close(clnt_sock);
    printf("client link finish: IP%s\n", inet_ntoa(clnt_addr.sin_addr));
    close(serv_sock);
    return 0;
}
    
