#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h> //important
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h> //best way to check nickname validity


#define MAX_EVENTS 100
#define MAX_CLIENTS 500
#define PORT 15579
#define MSG_LIMIT 200 //message length limit
#define NICK_LIMIT 16 //nick len 

typedef struct{
	int fd;
	char nickname[32]; //nick
	int registered;    //0: waiting for nickname 1: on chatting
} Client;

Client *clients[MAX_CLIENTS]; //indexnum means fd number so find quickly

int is_valid_nickname(char *nick){
	int len = strlen(nick);
	if (len < 4 || len > 16) return 0;

	for (int i = 0; i < len; i++){
		if (!isalpha(nick[i])) return 0;
	}
	return 1;
}

void setnonblocking(int sockfd) {
	int opts = fcntl(sockfd, F_GETFL);
	opts = (opts | O_NONBLOCK);
	fcntl(sockfd, F_SETFL, opts);
}

void send_to_all(int sender_fd, char *msg){
	for (int i = 0; i < MAX_CLIENTS; i++){
		if (clients[i] != NULL && clients[i]->registered == 1 && clients[i]->fd != sender_fd) {
			write(clients[i]->fd, msg, strlen(msg));
		}
	}
}

//to avoid deadlock, non-automatically delete client
void disconnect_client(int fd) {
	if (clients[fd] != NULL){
		if (clients[fd]->registered){
			char bye_msg[256];
			snprintf(bye_msg, sizeof(bye_msg), "[System] %s has been left.\n", clients[fd]->nickname);
			send_to_all(fd, bye_msg);
			printf("%s", bye_msg); //log for server
		}

		close(fd);
		free(clients[fd]);
		clients[fd] = NULL; //erase on clients list to make send logic right 
	}
}



int main() {
	int server_fd, epoll_fd;
	struct sockaddr_in addr;
	struct epoll_event ev, events[MAX_EVENTS];

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //without it, it would be very hard for server.. so important thing
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
	       	perror("socket Binding failed");
		exit(1);
	}
	listen(server_fd, 100);//second parameter no uimi.. 
	setnonblocking(server_fd);	
	epoll_fd = epoll_create1(0);
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

	printf("Tiny chat server is running on the port %d\n", PORT);

	while (1){
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		for (int i = 0; i < nfds; i++){
			int curr_fd = events[i].data.fd;
			//new accept of client
			if (curr_fd == server_fd){
				struct sockaddr_in client_addr;
				socklen_t addr_len = sizeof(client_addr);
				int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
				if (new_socket >= MAX_CLIENTS){
					printf("Error! Server is full"); //for server
					char *full_msg = "server is full.. please try again later....\n";
					write(new_socket, full_msg, strlen(full_msg));
					close(new_socket);
					continue;
				}

				setnonblocking(new_socket);

				ev.events = EPOLLIN; //Level Triggered
				ev.data.fd = new_socket;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev);
				clients[new_socket] = (Client *)malloc(sizeof(Client));
				clients[new_socket]->fd = new_socket;
				clients[new_socket]->registered = 0; //set nick plz
				char *welcome = "you should input nickname for using(length 4~16 for english): ";
				write(new_socket, welcome, strlen(welcome));
				printf("New Connection: FD %d\n", new_socket);
			}
			else {
				char buffer[512] = {0};
				int len = read(curr_fd, buffer, sizeof(buffer) - 1);
				if (len <= 0){
					disconnect_client(curr_fd);
				} else {
					buffer[strcspn(buffer, "\r\n")] = 0;
					Client *user = clients[curr_fd];
					if (!user) continue;
					if (user->registered == 0){ 
						if (strlen(buffer) < 1) continue;	
						if (is_valid_nickname(buffer) == 0){
							char *err = "you should input right nickname... connect us again";
							write(curr_fd, err, strlen(err));
							continue;
						}
						strncpy(user->nickname, buffer, NICK_LIMIT);
						user->registered = 1; // 0 -> 1 (cant back 0 again: no nickchange)
						char msg[256];
						snprintf(msg, sizeof(msg), "your nickname is [%s]. start chatting!\n", user->nickname);
						write(curr_fd, msg, strlen(msg));
						snprintf(msg, sizeof(msg), "[System] %s has been joined to server.\n", user->nickname);
						send_to_all(curr_fd, msg);
					}
					else {
						//sending message
						if (strlen(buffer) > MSG_LIMIT){
							char *err = "message is too long(you need to shorter than 200\n";
							write(curr_fd, err, strlen(err));
						}
						else {
							char chat_msg[512];
							snprintf(chat_msg, sizeof(chat_msg), "%s : %s\n", user->nickname, buffer);
							send_to_all(curr_fd, chat_msg);
						}
					}
				}
			}
		}
	}
	close(server_fd);
	return 0;

}
