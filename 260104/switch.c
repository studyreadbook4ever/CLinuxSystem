#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

/*opened port maximum */
#define MAX_PORTS 16

/*open socket for Promiscuous Mode. return socket's number dev=name of ifr*/
int init_port(char *dev){
	int sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) return -1;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	ioctl(sock, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_UP | IFF_PROMISC;
	ioctl(sock, SIOCSIFFLAGS, &ifr);

	/*socket binding*/
	ioctl(sock, SIOCGIFINDEX, &ifr);
	struct sockaddr_ll sll;
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(ETH_P_ALL);
	bind(sock, (struct sockaddr *)&sll, sizeof(sll));

	return sock;

}

int main() {
	printf("\033[H\033[J"); /* clear screen */
	printf("Virtual L2 Switch Firmware Loading...\n");
	int ports[MAX_PORTS];
	int active_ports = 0; /*activated port's number is saved */
	char dev_name[10];
	
	/*port opening */
	for (int i = 0; i < MAX_PORTS; i++){
		sprintf(dev_name, "eth%d", i); /*eth0-eth1-eth2... lancard making */
		int sock = init_port(dev_name);

		if (sock >= 0){
			ports[active_ports] = sock;
			printf("Port %d attached: %s (FD number: %d)\n", active_ports, dev_name, sock);
			active_ports++;
		} else {
			continue; /*cant open-> pass it */
		}
	}
	unsigned char buffer[65536];
	/*N:N Bridging(Flooding) */

	while(1){
		int traffic_detected = 0;

		for (int i = 0; i < active_ports; i++){
			/*DONTWAIT Logic is very important.. */
			int len = recv(ports[i], buffer, sizeof(buffer), MSG_DONTWAIT);
			if (len > 0) {
				traffic_detected = 1;
				for (int j = 0; j < active_ports; j++){
					if (i != j){
						send(ports[j], buffer, len, 0);
					}
				}
			}
		}
		if(traffic_detected != 1) usleep(100); /*polling for 10000m/s */
	}
	return 0;
}
