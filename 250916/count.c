#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        char *usagemsg = "usage: ./count filename\n";
        write(2, usagemsg, strlen(usagemsg));
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("file opening failure");
        return 1;
    }

    char buffer[BUFFER_SIZE]; 
    int bytes_read;
    int wordcount = 0;
    int in_word = 0;//now inside word-1 else-0

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            char c = buffer[i];
            if (c == ' ' || c == '\n' || c == '\t') {
               in_word = 0;
            }
            //A word ends -> B word starts and word_count++
            else if (in_word == 0) { 
                in_word = 1; // new word starts
                wordcount++;
            }
        }
    }

    if (bytes_read == -1) {
        perror("file reading failure");
        close(fd);
        return 1;
    }


    printf("%d\n", wordcount);
    close(fd);
    return 0;
}
