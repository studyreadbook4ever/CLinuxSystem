#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket()");
        exit(1);
    }
    printf("socket successfully created!\n");
    printf("gotten socket fd number: %d\n", sock);

    close(sock);
    return 0;
}
