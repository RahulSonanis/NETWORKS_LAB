/*
	Computer Networks Laboratory
	Assignment - 3 (RTLP Protocol Implementation Header)
	Client side Code
	Rahul Sonanis	13CS10049
	Vishwas Jain	13CS10053
*/


#ifndef RTLP_CLIENT_H
#define RTLP_CLIENT_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define BUFF_SIZE 1024
#define S_PORT    32302
#define D_PORT 	  30000

struct rtlp_header{
	int 		checksum;
	short int 	srcport;
	short int 	desport;
	int 		seqno;
	int 		ackno;
	short int   option;    /* SYN = 1    ACK = 2     DATA = 3     FIN = 4 */
};


extern int err;
extern int sockfd;
extern struct sockaddr_in daddr;
extern struct sockaddr_in saddr;
extern struct timeval tv;
extern struct rtlp_header info_header;
extern char DEST[100];
extern char SRC[100];

int   csum(int *, int );
void  printing(struct rtlp_header );
int   rtlp_connect(int,char *,char *);
int   rtlp_send(int ,char* );
void  receive_data(int , char* );
void  set_iphdr(char* );
void  set_rtlp_header(char* , int, int , short int , int );
int   rtlp_terminate(int sockf);

#endif