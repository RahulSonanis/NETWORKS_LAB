/*  
	CPP for the functions implemented for 
	the RTLP
	
	Name	- Vishwas Jain
	Roll 	- 13CS10053
	Name	- Rahul Sonanis
	Roll	- 13CS10049

	Assignment	- 3
	Developing a Reliable Transport
	Layer Protocol using Raw Sockets
*/





#include "RTLP_server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>



rtlp_hdr	rtlp_header;
unsigned long	client_ip_addr;
short int des_port;

void listenAccept(int sockfd){

	struct sockaddr_in client_addr;
	socklen_t fromlen = sizeof(client_addr);
	int sent_seqNo;
	char packet[BUFF_SIZE];
	
	rtlp_hdr* received_header;

	while(1){
		memset(packet, 0, sizeof(packet));
		if (recvfrom(sockfd, (char *)&packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &fromlen) < 0)
			perror("packet receive error:");

		received_header = (struct rtlp_hdr *)&packet[20];
		//printf("\n Received RTLP HEADER\n");
		//printRTLPHDR(received_header);
		//printf("\nCalculated checksum = %d\n", csum((int *)&packet[24], 250));
		if(csum((int *)&packet[24], 250) != received_header->checksum)	continue;
		
		if(received_header->option != SYN)		continue;

		break;
	}
	

	
	//Client requests for connection establishment, 3-way handshaking starting
	char syn_ack_packet[1024];
	memset(syn_ack_packet, 0, sizeof(syn_ack_packet));

	client_ip_addr = client_addr.sin_addr.s_addr;
	rtlp_header.src_port = PORT;
	rtlp_header.des_port = received_header->src_port;
	rtlp_header.seq_no = rand()%4096+1;
	rtlp_header.ack_no = received_header->seq_no;
	rtlp_header.option = ACK;
	set_iphdr(syn_ack_packet, client_ip_addr);
	set_rtlphdr(&syn_ack_packet[20], received_header->src_port, rtlp_header.seq_no, received_header->seq_no, rtlp_header.option);
	
	
	while(1){
		if (sendto(sockfd, (char *)syn_ack_packet, sizeof(syn_ack_packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
			perror("packet send error:");

		fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(sockfd, &rd);

	    struct timeval tv;

	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    if(select(sockfd + 1, &rd, NULL, NULL, &tv) == -1)
	    {
	    	perror("Error in select");
	    }

	    if(FD_ISSET(sockfd, &rd))
	   	{
	   		memset(packet, 0, sizeof(packet));
			if (recvfrom(sockfd, (char *)&packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &fromlen) < 0)
				perror("packet receive error:");


			received_header = (struct rtlp_hdr *)&packet[20];
			//printf("\n Received RTLP HEADER in while loop\n");
			//printRTLPHDR(received_header);

			//printf("checkosum cla = %d\n",csum((int *)&packet[24], 250) );
			if(csum((int *)&packet[24], 250) == received_header->checksum && received_header->option == ACK && received_header->ack_no == rtlp_header.seq_no)
				break;
	   	}
	}
}



 void receive_data(int sockfd, char* buff){
	struct sockaddr_in client_addr;
	socklen_t fromlen = sizeof(client_addr);
	char packet[BUFF_SIZE];
	
	rtlp_hdr* received_header;


	while(1){

		memset(packet, 0, sizeof(packet));
		if (recvfrom(sockfd, (char *)&packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &fromlen) < 0)
			perror("packet receive error:");

		//printf("\nIn receive function\n");
		received_header = (struct rtlp_hdr *)&packet[20];

		//printf("\n Received RTLP HEADER\n");
		//printRTLPHDR(received_header);

		//printf("\nCalculated checksum = %d\n", csum((int *)&packet[24], 250));
		if(csum((int *)&packet[24], 250) != received_header->checksum)	continue;
		

		char ack_packet[1024];
		memset(ack_packet, 0, sizeof(ack_packet));

		set_iphdr(ack_packet, client_ip_addr);
		rtlp_header.option = ACK;
		if(received_header->option == DATA){
			if(received_header->seq_no <= rtlp_header.ack_no){
				printf("\n\nDuplicate Data Received\n");
				set_rtlphdr(&ack_packet[20], received_header->src_port, rtlp_header.seq_no, rtlp_header.ack_no, rtlp_header.option);

				if (sendto(sockfd, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
					perror("packet send error:");
				continue;

			}
			else{

				strcpy(buff, &packet[38]);
				rtlp_header.ack_no = received_header->seq_no;
				set_rtlphdr(&ack_packet[20], received_header->src_port, rtlp_header.seq_no, rtlp_header.ack_no, rtlp_header.option);

				if (sendto(sockfd, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
					perror("packet send error:");
				break;
			}	
		}
		else if(received_header->option == FIN){
			char fin_ack_packet[1024];
			memset(fin_ack_packet, 0, sizeof(fin_ack_packet));

			client_ip_addr = client_addr.sin_addr.s_addr;
			rtlp_header.src_port = PORT;
			rtlp_header.des_port = received_header->src_port;
			rtlp_header.seq_no = 0;
			rtlp_header.ack_no = received_header->seq_no;
			rtlp_header.option = ACK;

			set_iphdr(fin_ack_packet, client_ip_addr);
			set_rtlphdr(&fin_ack_packet[20], received_header->src_port, rtlp_header.seq_no, rtlp_header.ack_no, rtlp_header.option);
			
			
			while(1){
				if (sendto(sockfd, (char *)fin_ack_packet, sizeof(fin_ack_packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
					perror("packet send error:");

				fd_set rd, wr, er;
			    FD_ZERO(&rd);
			    FD_ZERO(&wr);
			    FD_ZERO(&er);
			    FD_SET(sockfd, &rd);

			    struct timeval tv;

			    tv.tv_sec = 1;
			    tv.tv_usec = 0;
			    if(select(sockfd + 1, &rd, NULL, NULL, &tv) == -1)
			    {
			    	perror("Error in select");
			    }

			    if(FD_ISSET(sockfd, &rd))
			   	{
			   		memset(packet, 0, sizeof(packet));
					if (recvfrom(sockfd, (char *)&packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &fromlen) < 0)
						perror("packet receive error:");


					received_header = (struct rtlp_hdr *)&packet[20];
					//printf("\n Received RTLP HEADER in while loop\n");
					//printRTLPHDR(received_header);

					//printf("checkosum cla = %d\n",csum((int *)&packet[24], 250) );
					if(csum((int *)&packet[24], 250) == received_header->checksum && received_header->option == ACK){
						printf("\nConnection closed using three way handshaking\n");
						close(sockfd);
						exit(0);
						return;
					}
			   	}
			}

		}
	}
}



 void printRTLPHDR(rtlp_hdr* received_header){
	
	printf("\tsrcport  = %d\n",received_header->src_port);
	printf("\tdesport  = %d\n",received_header->des_port);
	printf("\tseqno    = %d\n",received_header->seq_no);
	printf("\tackno    = %d\n",received_header->ack_no);
	printf("\tchecksum = %d\n",received_header->checksum);

}


 void set_iphdr(char* buff, unsigned long clinet_IP){
	struct iphdr *ip = (struct iphdr *)buff; 
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = htons(1024);					/* 16 byte value */
	ip->frag_off = 0;							/* no fragment */
	ip->ttl = 64;								/* default value */
	ip->protocol = IPPROTO_RAW;					/* protocol at L4 */
	ip->check = 0;								/* not needed in iphdr */
	ip->saddr = 0;
	ip->daddr = clinet_IP;
}


 void set_rtlphdr(char* buff, short int des_port, int seq_no, int ack_no, short int option){
	rtlp_hdr* rtlp 	= (struct rtlp_hdr *)buff;

	rtlp->src_port 	= PORT;
	rtlp->des_port 	= des_port;
	rtlp->seq_no 	= seq_no;
	rtlp->ack_no	= ack_no;
	rtlp->option	= option;
	rtlp->checksum 	= csum((int *)&buff[4], 250);
	rtlp_header.checksum = rtlp->checksum;
	//printf("\n Sent RTLP HEADER\n");
	//printRTLPHDR(rtlp);
}


 int csum(int *buf, int nwords)
{      
        long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (int)(~sum);
}




int rtlp_send(int sockf,char* buffsend)
{
	struct sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;
	client_addr.sin_addr.s_addr = client_ip_addr;
	char packet[BUFF_SIZE];

	set_iphdr(packet, client_ip_addr);

	// Sending DATA packet
	rtlp_header.seq_no += strlen(buffsend);
	strcpy(&packet[38],buffsend);
	
	rtlp_header.option = 3;
	set_rtlphdr(&packet[20],rtlp_header.des_port, rtlp_header.seq_no,rtlp_header.ack_no, rtlp_header.option);


	if (sendto(sockf, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
		perror("packet send error:");

	

    while(1)
    {
    	// Waiting for ACK packet
	    fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(sockf, &rd);
    	struct timeval tv;

    	tv.tv_sec = 0;
	    tv.tv_usec = 200000;

	    if(select(sockf + 1, &rd, NULL, NULL, &tv) == -1)
	    {
	    	perror("Error in select");
	    	return 0;
	    }

	   	if(FD_ISSET(sockf, &rd))
	   	{
	   		char synackpacket[BUFF_SIZE];
	   		socklen_t fromlen = sizeof(struct sockaddr_in);

	   		if(recvfrom(sockf, (char *)&synackpacket, sizeof(synackpacket), 0,(struct sockaddr *)&client_addr, &fromlen) < 0)
	   		{
				perror("packet receive error:");
				return 0;
	   		}

	   		struct rtlp_hdr *received_header = (struct rtlp_hdr *)&synackpacket[20];
			//printf("\nReceived ACK HEADER\n");
			//printRTLPHDR(received_header);
			
			//printf("\ncalculated checksum = %d",csum((int *)&synackpacket[24],250));

			if(received_header->checksum == csum((int *)&synackpacket[24],250))
			{
				if(received_header->option == ACK){
					if(received_header->ack_no == rtlp_header.seq_no)
					{
						//printf("Correct ACK packet Received");
						rtlp_header.seq_no    =   received_header->ack_no;
						rtlp_header.ack_no    =   received_header->seq_no;
						break;
					}
					else continue;
				}
				else if(received_header->option == DATA){
					char ack_packet[1024];
					memset(ack_packet, 0, sizeof(ack_packet));

					set_iphdr(ack_packet, client_ip_addr);
					rtlp_header.option = ACK;
					printf("\n\nDuplicate Data Received\n");
					set_rtlphdr(&ack_packet[20], received_header->src_port, received_header->ack_no, rtlp_header.ack_no, rtlp_header.option);

					if (sendto(sockf, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
						perror("packet send error:");
					continue;
				}
				else
				{
					//printf("\nSending DATA packet again(wrong seqno and ackno)\n");
					//printRTLPHDR(&rtlp_header);

					if (sendto(sockf, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
						perror("packet send error:");
					continue;
				}
			}
			else
			{
				//printf("\nSending DATA Packet (wrong checksum)\n");
				//printRTLPHDR(&rtlp_header);

				if (sendto(sockf, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
					perror("packet send error:");
				continue;
			}
	   	}
	   	else
	   	{
			//printf("\nSending DATA packet again (Timeout)\n");
			//printRTLPHDR(&rtlp_header);

			if (sendto(sockf, (char *)packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr)) < 0)
				perror("packet send error:");
			//printf("Timeout\n");
	   	}
    }
    return 1;
}