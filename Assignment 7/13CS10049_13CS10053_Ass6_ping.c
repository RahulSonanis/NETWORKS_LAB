/*
		Rahul Sonanis     13CS10049
		Vishwas Jain      13CS10053
		Computer Networks Laboatory
		Assignment - 6
		Ping using raw Sockets
*/


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <ifaddrs.h>
#include <math.h>
extern int h_errno;


#define BUFF_SIZE       1024
#define ECHO_REPLY      0
#define DEST_UNREACH    3
#define REDIRECT        5
#define ECHO_REQUEST    8
#define TIME_EXCEEDED   11
#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))
void set_iphdr(char* , unsigned long, int );
void initalise_raw_socket();
void set_icmphdr(unsigned short);
void get_my_ip();
void get_host_ip();
void start_ping();
void get_host_name();
void set_icmppayload();


double time_dev[1000000];
char ping_input[BUFF_SIZE];
int raw_socket;
char buffer_for_sending[BUFF_SIZE], buffer_for_receiving[BUFF_SIZE];
struct sockaddr_in client_addr, daddr, saddr;
char* my_ip, *host_ip, *host_name;
clock_t send_time,recv_time;

double min_rtt,max_rtt,avg_rtt;
int sent_packets,received_packets;

struct timeval start_time,end_time;

unsigned short csum (unsigned short *buf, int nwords)
{
	unsigned long sum;
	for (sum = 0; nwords > 0; nwords--)
	sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

int main(int argc, char* argv[], char* env[]){

	gettimeofday(&start_time, NULL);
	if(argc != 2){
	    printf("Usage: ./ping IP-ADDRESS\n");
	    exit(0);
	}

	strcpy(ping_input,argv[1]);

	get_my_ip();
	get_host_ip(argv[1]);
	initalise_raw_socket();
	get_host_name(daddr.sin_addr);
	// if(host_name != NULL){
	//     printf("PING %s(%s)\n",host_name, inet_ntoa(daddr.sin_addr));
	// }
	// else{
	//     printf("PING %s(%s)\n",inet_ntoa(daddr.sin_addr), inet_ntoa(daddr.sin_addr));
	// }
	printf("PING %s (%s)\n",argv[1], inet_ntoa(daddr.sin_addr));
	start_ping();

	return 0;
}

void handler(int signum){

	int i;
	double avg = avg_rtt/received_packets;
	double mean_dev;

	for(i = 1 ; i <= received_packets ; i++)
	{
		mean_dev += fabs(time_dev[i]-avg);
	}
	mean_dev /= received_packets;

	gettimeofday(&end_time, NULL);
	suseconds_t difference = 1000000*end_time.tv_sec + end_time.tv_usec -1000000*start_time.tv_sec - start_time.tv_usec;
	double diff = (double)(difference)/1000;
	int dif = (int)diff;
	printf("\n--- %s ping statistics ---\n",ping_input);
	int packet_loss = sent_packets - received_packets;
	printf("%d packets transmitted, %d received, %d%% packet loss, time %dms\n",sent_packets,received_packets,(int)((((double)packet_loss)/sent_packets)*100),dif);
	if(min_rtt != 1000000000 && max_rtt != 0 && avg_rtt != 0)
	{
	    printf("rtt min/avg/max/mdev = %0.3f/%0.3f/%0.3f/%0.3f ms\n",min_rtt,avg,max_rtt,mean_dev);
	}
	close(raw_socket);
	exit(0);
}

void start_ping(){      
	max_rtt = 0;
	min_rtt = 1000000000;
	avg_rtt = 0;
	sent_packets = 0;
	received_packets = 0;
	int current_ttl_value = 48;
	int sequence_no = 1;
	struct icmphdr *icmphd2;

	signal(SIGINT,handler);

	while(1){
	    sleep(1);
	    set_iphdr(buffer_for_sending, daddr.sin_addr.s_addr, current_ttl_value);
	    set_icmppayload();
	    set_icmphdr(sequence_no);

	    struct timeval recv_time;
	    if(sendto(raw_socket, buffer_for_sending, 44, 0, (struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0){
	          perror("Unable to send: ");
	    }
	    sent_packets++;

	    int nfds = 0;
	    fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(raw_socket, &rd);
	    nfds = max(nfds, raw_socket);
	    struct timeval tv;
	    tv.tv_sec = 10;
	    tv.tv_usec = 0;
	    int readyFDCount = select(nfds + 1, &rd, NULL, NULL, &tv);
	    if(readyFDCount == -1){
	          perror("Error in select");
	          exit(0);
	    }
	    if (FD_ISSET(raw_socket, &rd)){

	          memset(buffer_for_receiving, 0, sizeof(buffer_for_receiving));
	          socklen_t fromlen = sizeof(client_addr);
	          int size = recvfrom(raw_socket, (char *)&buffer_for_receiving, sizeof(buffer_for_receiving), 0, (struct sockaddr *)&client_addr, &fromlen);
	          if (size < 0)
	                perror("packet receive error: ");
	          received_packets++;


	          struct timeval *tt = (struct timeval *) (buffer_for_receiving + 28);
	          gettimeofday(&recv_time, NULL);
	          suseconds_t difference = 1000000*recv_time.tv_sec + recv_time.tv_usec -1000000*(tt->tv_sec) - (tt->tv_usec);
	          double diff = (double)(difference)/1000;
	          icmphd2 = (struct icmphdr *) (buffer_for_receiving + 20);

	          if(diff > max_rtt)      max_rtt = diff;
	          if(diff < min_rtt)      min_rtt = diff;
	          avg_rtt += diff;
	          time_dev[received_packets] = diff;

	          get_host_name(client_addr.sin_addr);

	          if(host_name != NULL){
	                printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%f ms\n",size,host_name, inet_ntoa(client_addr.sin_addr), icmphd2->un.echo.sequence, current_ttl_value, diff);
	          }
	          else{
	                printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%f ms\n",size,inet_ntoa(client_addr.sin_addr), inet_ntoa(client_addr.sin_addr), icmphd2->un.echo.sequence, current_ttl_value, diff);
	          }
	    }
	    sequence_no++;
	}
}


void get_my_ip(){
	struct ifaddrs *addrs, *tmp;
	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
	    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
	    {
	          struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
	          if(!strcmp(tmp->ifa_name,"enp3s0")){
	                my_ip = strdup(inet_ntoa(pAddr->sin_addr));
	                break;
	          }

	    }
	    tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);
}


void get_host_ip(char* host_name){
	struct hostent *myip;
	if((myip = gethostbyname(host_name)) == NULL || myip->h_addrtype != AF_INET){
	    herror("Unable to get the host ip: ");
	    exit(0);
	}
	host_ip = strdup(inet_ntoa(*((struct in_addr *)myip->h_addr)));
}

void get_host_name(struct in_addr host_addr){
	struct hostent *myip;
	if((myip = gethostbyaddr(&host_addr,sizeof(struct in_addr), AF_INET)) == NULL){
	    // herror("Unable to get the host name: ");
	    host_name = NULL;
	}
	else{
	    host_name = strdup(myip->h_name);
	}
}

void initalise_raw_socket(){
	raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(raw_socket == -1){
	    perror("Unable to create raw socket: ");
	    exit(0);
	}
	int on=1;
	if(setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1){
	    perror("Unable to enable IP_HDRINCL: ");
	    exit(0);
	}

	daddr.sin_family = AF_INET;
	saddr.sin_family = AF_INET;
	daddr.sin_port = 0; /* not needed in SOCK_RAW */
	saddr.sin_port = 0; /* not needed in SOCK_RAW */
	if(inet_pton(AF_INET, my_ip, (struct in_addr *)&saddr.sin_addr.s_addr) == 0){
	    printf("Invalid IP Address\n");
	    exit(0);
	}
	if(inet_pton(AF_INET, host_ip, (struct in_addr *)&daddr.sin_addr.s_addr) == 0){
	    printf("Invalid IP Address\n");
	    exit(0);
	}
	memset(daddr.sin_zero, 0, sizeof(daddr.sin_zero));
	memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));
}


void set_icmphdr(unsigned short sequence_no_){
	struct icmphdr *icmphd = (struct icmphdr *) (buffer_for_sending + 20);
	icmphd->type = ICMP_ECHO;
	icmphd->code = 0;
	icmphd->checksum = 0;
	icmphd->un.echo.id = 0;
	icmphd->un.echo.sequence = sequence_no_;
	icmphd->checksum = csum ((unsigned short *) (buffer_for_sending + 20), 12);
}


void set_iphdr(char* buff, unsigned long clinet_IP, int ttl_value){
	struct iphdr *ip = (struct iphdr *)buff;
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = 0;					/* 16 byte value */
	ip->frag_off = 0;					/* no fragment */
	ip->ttl = ttl_value;				/* default value */
	ip->protocol = IPPROTO_ICMP;			/* protocol at L4 */
	ip->check = 0;
	ip->saddr = 0;					/* not needed in iphdr */
	ip->daddr = clinet_IP;
}

void set_icmppayload(){
	struct timeval *tt = (struct timeval *) (buffer_for_sending + 28);
	gettimeofday(tt, NULL);
}