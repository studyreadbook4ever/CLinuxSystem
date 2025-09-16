#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 256
#define MAX_ARGS 32

int main(int argc, char *argv[]) {
    char cmd[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    pid_t pid;

    while(1) {
        printf("myshell> ");

        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
             break; // how to insert NULL-> Ctrl + D
        }
        cmd[strcspn(cmd, "\n")] = 0;

        char *token;
        int i = 0;
        token = strtok(cmd," ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            printf("finish shell\n");
            break;
        }

        pid = fork();

        if (pid < 0) {
            perror("fork failure");
            exit(1);
        } else if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("execution failure"); // execvp<-- shell merei suhen
                exit(127);
            }
        } else {
            int status;
            wait(&status); // ending status 
        }
    }
    return 0;
}

