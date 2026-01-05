#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>

/*dev:name of this virtual card, ip: name of this  */
void set_ip(char *dev, char *ip) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, dev, IFNAMSIZ); //put on mem
	struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr->sin_addr);
	ioctl(fd, SIOCSIFADDR, &ifr);

	ioctl(fd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
	ioctl(fd, SIOCSIFFLAGS, &ifr);
	close(fd);
	printf("Network configured: %s\n", ip);
}

int main(int argc, char *argv[]) {
	char *name = "Default-Pod";/*Default Name*/
	char *ip = "192.168.1.99";/*Default IP */
	printf("\033[H\033[J"); /* clear screen */
	printf("[Micro-Pod: %s] Booting....\n", name);
	if (argc >= 3){
		name = argv[1];
		ip = argv[2];
	} else {
		printf("Warning! No Arguments....usage: ./switch <name> <ip>\n");
	}
	set_ip("eth0", ip);
	while(1) {
		printf("[%s] Service Running... (IP:%s)\n", name, ip);
		sleep(10);
	}
	return 0;
}	
