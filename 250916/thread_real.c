#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

//we need to communicate with two thread by mutex 

sem_t sem;
pthread_mutex_t stdin_mutex;
char msg[1024];

void* writer_thread(void* arg) {
    char local_buffer[1024];
    pthread_mutex_lock(&stdin_mutex);
    printf("Writer: input message plz: ");
    fgets(local_buffer, sizeof(local_buffer), stdin);
    local_buffer[strcspn(local_buffer, "\n")] = 0; 
    pthread_mutex_unlock(&stdin_mutex); //we need to return mutex key
    strcpy(msg, local_buffer);
    sem_post(&sem);
    return NULL;
}

void* reader_thread(void* arg) {
    sem_wait(&sem); //wait until writer finish task.(sem_init.value 0->1)
    printf("\nReader: gotten message -> %s\n", msg);
    return NULL;
}

int main() {
    pthread_t writer_tid, reader_tid;
    sem_init(&sem, 0, 0);
    pthread_mutex_init(&stdin_mutex, NULL); //mutex new

    printf("Writer and Reader's thread new creation.\n");
    pthread_create(&writer_tid, NULL, writer_thread, NULL);
    pthread_create(&reader_tid, NULL, reader_thread, NULL);

    pthread_join(writer_tid, NULL);
    pthread_join(reader_tid, NULL);
    sem_destroy(&sem);
    pthread_mutex_destroy(&stdin_mutex);

    printf("program finished.\n");
    return 0;
}
