#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h> 
#define BUFFER_SIZE 1024

//we can connect many client by only multiprocessing(high overhead)

void error_handling(char *message) { fputs(message, stderr); fputc('\n', stderr); exit(1); }

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    pid_t pid;
    char buffer[BUFFER_SIZE];
    int str_len;
    if (argc != 2) { printf("usage: %s <port>\n", argv[0]); exit(1); }
    signal(SIGCHLD, SIG_IGN);//ignore for childprocess terminate
    serv_sock = socket(PF_INET, SOCK_STREAM, 0); //TCP Internet
    //same as accept_socket.c 
    int option = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind() error");
    if (listen(serv_sock, 5) == -1) error_handling("listen() error");
    printf("multiprocessing server start... port %s\n", argv[1]);
    while(1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(clnt_sock == -1) { continue; }
        else { printf("new client link: %s\n", inet_ntoa(clnt_addr.sin_addr)); }
        pid = fork(); // parent: only linking, when link-make child for ungdae
        if (pid == -1) { close(clnt_sock); error_handling("process creation failure!!!!"); return 1; } // when fork fail->bad error 
        if (pid == 0) {
            close(serv_sock);
            int str_len;
            char nick[12];
            char welcom_msg[100];
            char buffer[BUFFER_SIZE];
            char echo_buffer[BUFFER_SIZE + 12];
            write(clnt_sock, "input nickname plz: < 12", strlen("input nickname plz: < 12"));
            str_len = read(clnt_sock, nick, sizeof(nick) - 1);
            nick[str_len] = 0; //input Null by no-auto
            char* p = strchr(nick, '\n');
            if (p) *p = '\0';
            sprintf(welcom_msg, "'%s', Hello! We are now starting echo service...\n", nick);
            write(clnt_sock, welcom_msg, strlen(welcom_msg));

            while((str_len = read(clnt_sock, buffer, BUFFER_SIZE)) != 0) {
                buffer[str_len] = 0;
                sprintf(echo_buffer, "[%s]: %s", nick, buffer);
                write(clnt_sock,echo_buffer,strlen(echo_buffer)); 
            }
            close(clnt_sock);
            printf("client link finish: %s\n", inet_ntoa(clnt_addr.sin_addr));
            exit(0);
        } else {
            close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;

}        
