#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>


int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "usage: %s <fsize(bytes)>\n", argv[0]);
        return 1;
    }
    long min_size = atol(argv[1]); //atoi means bulanjung
    
    DIR *dp = opendir(".");
    if (dp == NULL) {
        perror("directory opening failure");
        return 1;
    }

    struct dirent *entry; //dir's info (pointer)
    struct stat file_stat; // file's info

    while ((entry = readdir(dp)) != NULL) {
        if (stat(entry->d_name, &file_stat) == -1) {
            perror(entry->d_name);
            continue;
        }

       if (S_ISREG(file_stat.st_mode)) {
            if (file_stat.st_size > min_size) {
                printf("%s %ld\n", entry->d_name, (long)file_stat.st_size); 
            }
       }
    }


    closedir(dp);

    return 0;
}

    
