#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define EPOLL_SIZE 20
#define MAX_CLIENTS_PER_WORKER 100
#define NUM_CORES 2
#define NICK_LEN 12

typedef struct {
    int fd;
    char nickname[NICK_LEN];
} ClientState;

struct SharedData {
    pthread_mutex_t mutex;
    char broadcast_message[BUFFER_SIZE];
};

int shmid;
void* shm_ptr = (void*) - 1;

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void cleanup_resources(int sig) {
    printf("\nServer is down. Cleaning up memory...\n");
    if (shm_ptr != (void*)-1) shmdt(shm_ptr);
    if (shmid > 0) shmctl(shmid, IPC_RMID, NULL);
    kill(0, SIGKILL);
    exit(0);
}

void worker_process_loop(int listen_sock, int ipc_pipe[], struct SharedData* shared_data) {
    int epfd;
    struct epoll_event event;
    struct epoll_event *ep_events;
    ClientState client_jungbo[MAX_CLIENTS_PER_WORKER];

    for (int i = 0; i < MAX_CLIENTS_PER_WORKER; i++) {
        client_jungbo[i].fd = -1;
    }

    // FIX: Do not close the write-end of the pipe in worker processes.
    // All workers need to be able to write to it to signal others.
    // close(ipc_pipe[1]);

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = listen_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &event);

    event.events = EPOLLIN;
    event.data.fd = ipc_pipe[0];
    epoll_ctl(epfd, EPOLL_CTL_ADD, ipc_pipe[0], &event);

    printf("Worker PID: %d started...\n", getpid());

    while (1) {
        int event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        for (int i = 0; i < event_cnt; i++) {
            int current_fd = ep_events[i].data.fd;

            if (current_fd == listen_sock) {
                struct sockaddr_in clnt_addr;
                socklen_t addr_size = sizeof(clnt_addr);
                int clnt_sock = accept(listen_sock, (struct sockaddr*)&clnt_addr, &addr_size);
		sleep(1);//wait 1second gojung for stability
                
                write(clnt_sock, "What is your nickname?? : ", strlen("What is your nickname?? : "));
                
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                
                int j;
                for (j = 0; j < MAX_CLIENTS_PER_WORKER; j++) {
                    if (client_jungbo[j].fd == -1) {
                        // FIX: Use assignment (=) instead of comparison (==) to store the new client fd.
                        client_jungbo[j].fd = clnt_sock;
                        strcpy(client_jungbo[j].nickname, ""); // Initialize nickname
                        break;
                    }
                }
                printf("Worker %d accepted client: fd:%d\n", getpid(), clnt_sock);

            } else if (current_fd == ipc_pipe[0]) {
                char signal_buf[1];
                read(ipc_pipe[0], signal_buf, 1);
                
                pthread_mutex_lock(&shared_data->mutex);
                char temp_msg[BUFFER_SIZE];
                strcpy(temp_msg, shared_data->broadcast_message);
                pthread_mutex_unlock(&shared_data->mutex);

                for (int j = 0; j < MAX_CLIENTS_PER_WORKER; j++) {
                    if (client_jungbo[j].fd != -1) {
                        write(client_jungbo[j].fd, temp_msg, strlen(temp_msg));
                    }
                }
            } else {
                int client_index = -1;
                for (int j = 0; j < MAX_CLIENTS_PER_WORKER; j++) {
                    if (client_jungbo[j].fd == current_fd) {
                        client_index = j;
                        break;
                    }
                }
                if (client_index == -1) continue;

                char buf[BUFFER_SIZE];
                int str_len = read(current_fd, buf, BUFFER_SIZE);

                if (str_len <= 0) {
                    char exit_msg[BUFFER_SIZE];
                    if (strlen(client_jungbo[client_index].nickname) > 0) {
                        printf("Worker %d: Client '%s' disconnected: fd %d\n", getpid(), client_jungbo[client_index].nickname, current_fd);
                        sprintf(exit_msg, "[SYSTEM]: %s has left the chat.\n", client_jungbo[client_index].nickname);
                    } else {
                        printf("Worker %d: Unauthenticated client disconnected: fd %d\n", getpid(), current_fd);
                        sprintf(exit_msg, "[SYSTEM]: An unauthenticated user has left.\n");
                    }
                    
                    pthread_mutex_lock(&shared_data->mutex);
                    strcpy(shared_data->broadcast_message, exit_msg);
                    pthread_mutex_unlock(&shared_data->mutex);
                    write(ipc_pipe[1], "!", 1);

                    epoll_ctl(epfd, EPOLL_CTL_DEL, current_fd, NULL);
                    close(current_fd);
                    client_jungbo[client_index].fd = -1;
                    strcpy(client_jungbo[client_index].nickname, "");
                } else {
                    buf[str_len] = '\0';
                    
                    if (strlen(client_jungbo[client_index].nickname) == 0) {
                        char* p = strchr(buf, '\n');
                        if (p) *p = '\0';
                        
                        strncpy(client_jungbo[client_index].nickname, buf, NICK_LEN);
                        client_jungbo[client_index].nickname[NICK_LEN] = '\0';
                        
                        printf("Worker %d: fd %d's nickname set to '%s'\n", getpid(), current_fd, client_jungbo[client_index].nickname);

                        char join_msg[BUFFER_SIZE];
                        sprintf(join_msg, "[SYSTEM]: %s has joined the chat.\n", client_jungbo[client_index].nickname);

                        pthread_mutex_lock(&shared_data->mutex);
                        strcpy(shared_data->broadcast_message, join_msg);
                        pthread_mutex_unlock(&shared_data->mutex);
                        write(ipc_pipe[1], "!", 1);
                    } else {
                        char formatted_msg[BUFFER_SIZE + NICK_LEN + 4];
                        sprintf(formatted_msg, "[%s]: %s", client_jungbo[client_index].nickname, buf);
                        
                        pthread_mutex_lock(&shared_data->mutex);
                        strcpy(shared_data->broadcast_message, formatted_msg);
                        pthread_mutex_unlock(&shared_data->mutex);
                        write(ipc_pipe[1], "!", 1);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int listen_sock;
    struct sockaddr_in serv_addr;
    pid_t pid;
    int ipc_pipe[2];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    signal(SIGINT, cleanup_resources);
    
    key_t key = ftok("chat_shm_key", 'M');
    shmid = shmget(key, sizeof(struct SharedData), 0666 | IPC_CREAT);
    if (shmid == -1) error_handling("shmget error");
    
    shm_ptr = shmat(shmid, NULL, 0);
    struct SharedData* shared_data = (struct SharedData*)shm_ptr;

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_data->mutex, &mutex_attr);
    
    if (pipe(ipc_pipe) == -1) error_handling("pipe error");
    
    listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    if (bind(listen_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) error_handling("bind() error");
    if (listen(listen_sock, 4096) == -1) error_handling("listen error");
    
    printf("Master PID %d : Found %d cores. Creating workers...\n", getpid(), (long)NUM_CORES);
    
    for (int i = 0; i < NUM_CORES; i++) {
        pid = fork();
        if (pid == 0) {
            worker_process_loop(listen_sock, ipc_pipe, shared_data);
            exit(0);
        }
    }

    close(ipc_pipe[0]);
    close(ipc_pipe[1]);
    signal(SIGCHLD, SIG_IGN);
    
    printf("Master: All workers created....\n");
    while (1) {
        pause();
    }

    return 0;
}
