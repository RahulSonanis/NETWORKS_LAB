/*  
	Header file for the RTLP.
	
	Name	- Vishwas Jain
	Roll 	- 13CS10053
	Name	- Rahul Sonanis
	Roll	- 13CS10049

	Assignment	- 3
	Developing a Reliable Transport
	Layer Protocol using Raw Sockets
*/

#ifndef RTLP_SERVER_H
#define RTLP_SERVER_H

#define BUFF_SIZE 1024

#define	PORT    30000
#define SYN		1
#define ACK		2
#define DATA 	3
#define FIN		4


	
typedef struct rtlp_hdr
{
	int 		checksum;
	short int 	src_port;
	short int 	des_port;
	int 		seq_no;
	int 		ack_no;
	short int	option;   /* 1. SYN		2. ACK 		3.SYN+ACK 		4. DATA 		4.FIN*/
	
}rtlp_hdr;


int csum(int *, int );
void listenAccept(int);
void printRTLPHDR(rtlp_hdr* );
void set_iphdr(char* , unsigned long );
void set_rtlphdr(char* , short int , int , int, short int );
void receive_data(int , char* );
int rtlp_send(int ,char* );




extern rtlp_hdr	rtlp_header;
extern unsigned long	client_ip_addr;
extern short int des_port;

using namespace std;

#endif