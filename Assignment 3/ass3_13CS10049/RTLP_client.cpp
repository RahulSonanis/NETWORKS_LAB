/*
	Computer Networks Laboratory
	Assignment - 3 (RTLP Protocol Implementation )
	Client side Code
	Rahul Sonanis	13CS10049
	Vishwas Jain	13CS10053
*/


#include "RTLP_client.h"

#define SYN   1
#define ACK   2
#define DATA  3
#define FIN   4

int err;
struct sockaddr_in daddr;
struct sockaddr_in saddr;
struct timeval tv;
struct rtlp_header info_header;
char DEST[100];
char SRC[100];


void printing(struct rtlp_header test)
{
	printf("\tsrcport  = %d\n",test.srcport);
	printf("\tdesport  = %d\n",test.desport);
	printf("\tseqno    = %d\n",test.seqno);
	printf("\tackno    = %d\n",test.ackno);
	printf("\tchecksum = %d\n",test.checksum);
	printf("\toption   = %d\n",test.option);
}

int csum(int *buf, int nwords)
{       //
        long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum & 0xffff);
       	sum += (sum >> 16);
        return (int)(~sum);
}

void set_iphdr(char* buff)
{
	struct iphdr *ip = (struct iphdr *)buff; 

	daddr.sin_family = AF_INET;
	saddr.sin_family = AF_INET;
	daddr.sin_port = 0; /* not needed in SOCK_RAW */
	saddr.sin_port = 0; /* not needed in SOCK_RAW */
	inet_pton(AF_INET, SRC, (struct in_addr *)&saddr.sin_addr.s_addr);
	inet_pton(AF_INET, DEST, (struct in_addr *)&daddr.sin_addr.s_addr);
	memset(daddr.sin_zero, 0, sizeof(daddr.sin_zero));
	memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));

	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = htons(1024);					/* 16 byte value */
	ip->frag_off = 0;							/* no fragment */
	ip->ttl = 64;								/* default value */
	ip->protocol = IPPROTO_RAW;					/* protocol at L4 */
	ip->check = 0;								/* not needed in iphdr */
	ip->saddr = saddr.sin_addr.s_addr;
	ip->daddr = daddr.sin_addr.s_addr;
}


void set_rtlp_header(char* buff, int seq_no, int ack_no, short int option, int size)
{
	rtlp_header* hd = (struct rtlp_header *)&buff[20];

	hd->srcport 	= S_PORT;
	hd->desport 	= D_PORT;
	hd->seqno 	    = seq_no;
	hd->ackno	    = ack_no;
	hd->option      = option;
	hd->checksum 	= csum((int *)&buff[24],size);
	info_header.checksum = hd->checksum;
	// printf("\nPrinting header in function\n");
	// printing(*hd);
}


int rtlp_connect(int sockf,char src_addr[100],char dest_addr[100])
{
	strcpy(SRC,src_addr);
	strcpy(DEST,dest_addr);
	// Connection Phase
	info_header.srcport  = S_PORT;
	info_header.desport  = D_PORT;
	info_header.seqno    = rand()%4096 + 1;
	info_header.ackno    = 0;
	info_header.option   = SYN;

	// Sending SYN packet
	char packet[BUFF_SIZE];
	set_iphdr(packet);
	// printf("\nSent SYN packet HEADER\n");
	set_rtlp_header(packet,info_header.seqno,info_header.ackno,info_header.option,250);

	if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	{
		perror("packet send error:");
		return 0;
	}
	// Sent SYN packet

	// Waiting for SYN+ACK packet
    

    while(1)
    {
    	fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(sockf, &rd);

    	tv.tv_sec = 1;
	    tv.tv_usec = 0;

	    err = select(sockf + 1, &rd, NULL, NULL, &tv);
	    if(err == -1)
	    {
	    	perror("Error in select");
	    	return 0;
	    }

	   	if(FD_ISSET(sockf, &rd))
	   	{
	   		char synackpacket[BUFF_SIZE];
	   		socklen_t fromlen = sizeof(struct sockaddr_in);

	   		if(recvfrom(sockf, (char *)&synackpacket, sizeof(synackpacket), 0,(struct sockaddr *)&daddr, &fromlen) < 0)
	   		{
				perror("packet receive error:");
				return 0;
	   		}

	   		struct rtlp_header *hd1 = (struct rtlp_header *)&synackpacket[20];
			// printf("\nReceived SYN+ACK Packet\n");
			// printing(*hd1);

			// printf("\nCalculated checksum of Received SYN+ACK Packet = %d\n",csum((int *)&synackpacket[24],250));

			if(hd1->checksum == csum((int *)&synackpacket[24],250))
			{
				if(hd1->ackno == info_header.seqno)
				{
					// printf("Correct SYN+ACK packet Received\n");
					info_header.seqno   =  hd1->ackno;
					info_header.ackno   =  hd1->seqno;
					break;
				}
				else
				{
					// printf("\nSending SYN packet again (wrong seqno and ackno)\n");
					// printing(info_header);

					if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
					{
						perror("packet send error:");
						return 0;
					}
					continue;
				}
			}
			else
			{
				// printf("\nSending SYN+ACK packet again (wrong checksum)\n");
				// printing(info_header);

				if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
				{
					perror("packet send error:");
					return 0;
				}
				continue;
			}
	   	}
	   	else
	   	{
			// printf("\nSending SYN packet again (Timeout)\n");
			// printing(info_header);

			if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
			{
				perror("packet send error:");
				return 0;
			}
	   	}
    }

    // printf("\nSending ACK packet\n");

    info_header.option = ACK;
    // printf("Sent ACK packet\n");
	set_rtlp_header(packet, info_header.seqno, info_header.ackno, info_header.option, 250);

	// printf("Sent ACK packet\n");
	// printing(info_header);

	if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	{
		perror("packet send error:");
		return 0;
	}

	return 1;
}



int rtlp_send(int sockf,char* buffsend)
{
	char packet[BUFF_SIZE];
	set_iphdr(packet);

	// Sending DATA packet
	info_header.seqno += strlen(buffsend);
	info_header.option = DATA;
	strcpy(&packet[38],buffsend);

	// printf("\nSending DATA packet\n");
	set_rtlp_header(packet,info_header.seqno,info_header.ackno,info_header.option,250);

	// printf("\nSending DATA packet\n");
	// printing(info_header);

	if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	{
		perror("packet send error:");
		return 0;
	}

	// Waiting for ACK packet

    while(1)
    {
    	fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(sockf, &rd);

    	tv.tv_sec = 0;
	    tv.tv_usec = 200000;

	    err = select(sockf + 1, &rd, NULL, NULL, &tv);
	    if(err == -1)
	    {
	    	perror("Error in select");
	    	return 0;
	    }

	   	if(FD_ISSET(sockf, &rd))
	   	{
	   		char synackpacket[BUFF_SIZE];
	   		socklen_t fromlen = sizeof(struct sockaddr_in);

	   		if(recvfrom(sockf, (char *)&synackpacket, sizeof(synackpacket), 0,(struct sockaddr *)&daddr, &fromlen) < 0)
	   		{
				perror("packet receive error:");
				return 0;
	   		}

	   		struct rtlp_header *hd1 = (struct rtlp_header *)&synackpacket[20];
			// printf("\nReceived ACK Packet\n");
			// printing(*hd1);
			
			// printf("\nCalculated checksum of Received ACK Packet = %d\n",csum((int *)&synackpacket[24],250));

			if(hd1->checksum == csum((int *)&synackpacket[24],250))
			{
				if(hd1->option == ACK)
				{	//
					if(hd1->ackno == info_header.seqno)
					{
						// printf("\nCorrect ACK packet Received\n");
						info_header.seqno    =   hd1->ackno;
						info_header.ackno    =   hd1->seqno;
						break;
					}
					else	continue;
				}
				else if(hd1->option == DATA){
					char ack_packet[1024];
					memset(ack_packet, 0, sizeof(ack_packet));

					set_iphdr(ack_packet);
					info_header.option = ACK;
					printf("\n\nDuplicate Data Received send\n");
					set_rtlp_header(ack_packet,hd1->ackno, info_header.ackno, info_header.option,250);

					if (sendto(sockf, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
						perror("packet send error:");
					continue;
				}
				else
				{
					// printf("\nSending DATA packet again (wrong seqno and ackno)\n");
					// printing(info_header);

					if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
					{
						perror("packet send error:");
						return 0;
					}
					continue;
				}
			}
			else
			{
				// printf("\nSending DATA Packet again (wrong checksum)\n");
				// printing(info_header);

				if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
				{
					perror("packet send error:");
					return 0;
				}
				continue;
			}
	   	}
	   	else
	   	{
			// printf("\nSending DATA packet again (Timeout)\n");
			// printing(info_header);

			if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
			{
				perror("packet send error:");
				return 0;
			}
	   	}
    }
    return 1;
}


void receive_data(int sockf, char* buff)
{
	socklen_t fromlen = sizeof(struct sockaddr_in);;
	
	char recv_packet[BUFF_SIZE];

	while(1)
	{
		memset(recv_packet, 0, sizeof(recv_packet));
		
		if (recvfrom(sockf, (char *)&recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&daddr, &fromlen) < 0)
			perror("packet receive error:");

		struct rtlp_header *hd1 = (struct rtlp_header *)&recv_packet[20];

		// printf("\n Received DATA Packet\n");
		// printing(*hd1);

		// printf("\nCalculated checksum = %d\n", csum((int *)&recv_packet[24], 250));

		if(csum((int *)&recv_packet[24], 250) != hd1->checksum)	continue;
		
		char ack_packet[BUFF_SIZE];
		memset(ack_packet, 0, sizeof(ack_packet));

		set_iphdr(ack_packet);
		info_header.option = ACK;


		if(hd1->option == DATA)
		{
			if(hd1->seqno <= info_header.ackno)
			{
				printf("Duplicate Data Received receive\n");

				// printf("\n Sent ack Packet\n");
				set_rtlp_header(ack_packet, info_header.seqno, info_header.ackno,info_header.option,250);
				// printf("\n Sent ack Packet\n");
				// printing(info_header);

				if (sendto(sockf, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
					perror("packet send error:");

				continue;
			}
			else
			{
				strcpy(buff, &recv_packet[38]);
				info_header.ackno = hd1->seqno;

				// printf("\n Sent ack Packet\n");
				set_rtlp_header(ack_packet,info_header.seqno, info_header.ackno,info_header.option,250);

				// printf("\n Sent ack Packet\n");
				// printing(info_header);

				if (sendto(sockf, (char *)ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
					perror("packet send error:");
				break;
			}
		}	
	}
}


int rtlp_terminate(int sockf)
{
	// Connection Phase
	info_header.srcport  = S_PORT;
	info_header.desport  = D_PORT;
	info_header.seqno    = 0;
	info_header.option   = FIN;

	// Sending SYN packet
	char packet[BUFF_SIZE];
	set_iphdr(packet);
	// printf("\nSent SYN packet HEADER\n");
	set_rtlp_header(packet,info_header.seqno,info_header.ackno,info_header.option,250);

	if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	{
		perror("packet send error:");
		return 0;
	}
	// Sent FIN packet

	// Waiting for FIN+ACK packet
    while(1)
    {
    	fd_set rd, wr, er;
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_ZERO(&er);
	    FD_SET(sockf, &rd);

    	tv.tv_sec = 1;
	    tv.tv_usec = 0;

	    err = select(sockf + 1, &rd, NULL, NULL, &tv);
	    if(err == -1)
	    {
	    	perror("Error in select");
	    	return 0;
	    }

	   	if(FD_ISSET(sockf, &rd))
	   	{
	   		char finackpacket[BUFF_SIZE];
	   		socklen_t fromlen = sizeof(struct sockaddr_in);

	   		if(recvfrom(sockf, (char *)&finackpacket, sizeof(finackpacket), 0,(struct sockaddr *)&daddr, &fromlen) < 0)
	   		{
				perror("packet receive error:");
				return 0;
	   		}

	   		struct rtlp_header *hd1 = (struct rtlp_header *)&finackpacket[20];
			// printf("\nReceived FIN+ACK Packet\n");
			// printing(*hd1);

			// printf("\nCalculated checksum of Received FIN+ACK Packet = %d\n",csum((int *)&synackpacket[24],250));

			if(hd1->checksum == csum((int *)&finackpacket[24],250))
			{
				if(hd1->ackno == 0 && hd1->seqno == 0)
				{
					// printf("Correct FIN+ACK packet Received\n");
					info_header.seqno   =  hd1->ackno;
					info_header.ackno   =  hd1->seqno;
					break;
				}
				else
				{
					// printf("\nSending FIN packet again (wrong seqno and ackno)\n");
					// printing(info_header);

					if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
					{
						perror("packet send error:");
						return 0;
					}
					continue;
				}
			}
			else
			{
				// printf("\nSending FIN+ACK packet again (wrong checksum)\n");
				// printing(info_header);

				if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
				{
					perror("packet send error:");
					return 0;
				}
				continue;
			}
	   	}
	   	else
	   	{
			// printf("\nSending FIN packet again (Timeout)\n");
			// printing(info_header);

			if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
			{
				perror("packet send error:");
				return 0;
			}
	   	}
    }

    // printf("\nSending ACK packet\n");

    info_header.option = ACK;
    // printf("Sent ACK packet\n");
	set_rtlp_header(packet, info_header.seqno, info_header.ackno, info_header.option, 250);

	// printf("Sent ACK packet\n");
	// printing(info_header);

	if(sendto(sockf, (char *)packet, sizeof(packet), 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	{
		perror("packet send error:");
		return 0;
	}

	close(sockf);

	return 1;
}