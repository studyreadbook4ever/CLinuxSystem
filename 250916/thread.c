#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

sem_t sem; //semaphore for sync
char msg[1024];

void* writer_thread(void* arg) {
    printf("Writer: writing message...\n");
    strcpy(msg, "hello"); 
    sem_post(&sem);
    return NULL;
}

void* reader_thread(void* arg) {
    printf("Reader: waiting message...\n");
    sem_wait(&sem);
    printf("Reader: gotten message -> %s\n", msg);
    return NULL;
}

int main() {
    pthread_t writer_tid, reader_tid;

    //sem_init(&sem, 0, 0); chogihwa sem,share with threads,initial allowing == 0
    if (sem_init(&sem, 0, 0) == -1) {
        perror("sem_init");
        return 1;
    }

    pthread_create(&writer_tid, NULL, writer_thread, NULL);
    pthread_create(&reader_tid, NULL, reader_thread, NULL);

    //wait 'main' while threads finishing
    pthread_join(writer_tid, NULL);
    pthread_join(reader_tid, NULL); 
    sem_destroy(&sem);
    return 0;
}
