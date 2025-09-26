//shared memory expert code will be soon soon later cause we don't know epoll select yet.. 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>

int main () {
    key_t key; //making key
    if ((key = ftok("shm_basic", 'A')) == -1) { perror("ftok"); exit(1); }
    printf("1. key making success: %d\n", key);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT); //making shm
    if (shmid == -1) { perror("shmget"); exit(1); }
    printf("2. shm making success: %d\n", shmid);
    void* shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void*)-1) { perror("shmat"); exit(1); }
    printf("3. shared memory linked success! address: %p\n", shm_ptr);
    char* shared_buffer = (char*)shm_ptr;
    printf("4. writing data on shm(less overhead!!!!!!!!)...\n"); 
    strcpy(shared_buffer, "Hello, Shared Memory! This is a test.");
    printf(" -> writing finish. read data : '%s'\n", shared_buffer);
    if (shmdt(shm_ptr) == -1) { perror("shmdt"); exit(1); }
    printf("5. shared memory deletion complete!\n");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) { perror("shmctl"); exit(1); }
    printf("6. shm remove success!\n");
    return 0;
}
