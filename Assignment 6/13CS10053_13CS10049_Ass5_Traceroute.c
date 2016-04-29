/*
      Vishwas Jain      13CS10053
      Rahul Sonanis     13CS10049
      Computer Networks Laboatory
      Assignment - 5
      TraceRoute using ICMP Header
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
#include <ifaddrs.h>
extern int h_errno;

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */



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
void set_icmphdr();
void get_my_ip();
void get_host_ip();
void start_traceroute();
void get_host_name();


int raw_socket;
char buffer_for_sending[BUFF_SIZE], buffer_for_receiving[BUFF_SIZE];
struct sockaddr_in client_addr, daddr, saddr;
char* my_ip, *host_ip, *host_name;
clock_t send_time,recv_time;


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

      if(argc != 2){
            printf("Usage: ./traceroute IP-ADDRESS\n");
            exit(0);
      }

      get_my_ip();
      get_host_ip(argv[1]);
      initalise_raw_socket();
      printf(BOLDWHITE"\n\n\t    Traceroute to "BOLDBLUE"%s"BOLDGREEN" (%s)"BOLDWHITE", 30 hops max, 28 byte packets\n"RESET,argv[1],host_ip);
      printf(BOLDBLACK"--------------------------------------------------------------------------------------------\n"RESET);
      start_traceroute();

      return 0;
}

void start_traceroute(){

      int current_ttl_value = 1;
      struct icmphdr *icmphd2 ;
      while(1){
            set_iphdr(buffer_for_sending, daddr.sin_addr.s_addr, current_ttl_value);
            set_icmphdr();
            int host_found = 0;
            char times[3][100];
            int i;
            for ( i = 0; i < 3; i++) {
                  struct timeval start_time, recv_time;
                  gettimeofday(&start_time, NULL);
                  if(sendto(raw_socket, buffer_for_sending, 28, 0, (struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0){
                        perror("Unable to send: ");
                  }

                  int nfds = 0;
                  fd_set rd, wr, er;
                  FD_ZERO(&rd);
                  FD_ZERO(&wr);
                  FD_ZERO(&er);
                  FD_SET(raw_socket, &rd);
                  nfds = max(nfds, raw_socket);
                  struct timeval tv;
                  tv.tv_sec = 3;
                  tv.tv_usec = 0;
                  int readyFDCount = select(nfds + 1, &rd, NULL, NULL, &tv);

                  if(readyFDCount == -1){
                        perror("Error in select");
                        exit(0);
                  }
                  if (FD_ISSET(raw_socket, &rd)){
                        host_found = 1;
                        memset(buffer_for_receiving, 0, sizeof(buffer_for_receiving));
                        socklen_t fromlen = sizeof(client_addr);
                        if (recvfrom(raw_socket, (char *)&buffer_for_receiving, sizeof(buffer_for_receiving), 0, (struct sockaddr *)&client_addr, &fromlen) < 0)
                              perror("packet receive error: ");
                        gettimeofday(&recv_time, NULL);
                        suseconds_t difference = 1000000*recv_time.tv_sec + recv_time.tv_usec -1000000*start_time.tv_sec - start_time.tv_usec;
                        double diff = (double)(difference)/1000;
                        icmphd2 = (struct icmphdr *) (buffer_for_receiving + 20);
                        sprintf(times[i],"%lf",diff);
                  }
                  else{
                        host_found = 0;
                        break;
                  }
            }
            if(host_found == 1){
                  get_host_name(client_addr.sin_addr);
                  if(host_name != NULL){
                        printf(BOLDWHITE"\t%d"BOLDBLUE"\t%s"BOLDGREEN"\t(%s)"BOLDWHITE"\t%s ms\t%s ms\t%s ms\n"RESET,current_ttl_value, host_name, inet_ntoa(client_addr.sin_addr), times[0], times[1], times[2]);

                  }
                  else{
                        printf(BOLDWHITE"\t%d"BOLDBLUE"\t%s"BOLDGREEN"\t(%s)"BOLDWHITE"\t%s ms\t%s ms\t%s ms\n"RESET,current_ttl_value, inet_ntoa(client_addr.sin_addr), inet_ntoa(client_addr.sin_addr), times[0], times[1], times[2]);

                  }
                  if(icmphd2->type == DEST_UNREACH){
                        printf(RED"\t\t\t\tDestination Unreachable\n"RESET);
                  }
            }
            else{
                  printf("\t%d\t*\t*\t*\n",current_ttl_value);
            }
            if(icmphd2->type == ECHO_REPLY || icmphd2->type == DEST_UNREACH)  break;

            current_ttl_value++;
      }
      printf(BOLDBLACK"--------------------------------------------------------------------------------------------\n"RESET);
      printf("\n\n");
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


void set_icmphdr(){
      struct icmphdr *icmphd = (struct icmphdr *) (buffer_for_sending + 20);
      icmphd->type = ICMP_ECHO;
      icmphd->code = 0;
      icmphd->checksum = 0;
      icmphd->un.echo.id = 0;
      icmphd->un.echo.sequence = 0;
      icmphd->checksum = csum ((unsigned short *) (buffer_for_sending + 20), 4);
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
