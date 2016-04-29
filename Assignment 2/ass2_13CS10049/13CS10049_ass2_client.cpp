/*
	Computer Networks Laboratory
	Assignment - 2 Ticket Counter
	Client side code
	Rahul Sonanis	13CS10049
	Vishwas Jain	13CS10053
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> // For the socket (), bind () etc. functions.
#include <netinet/in.h> // For IPv4 data struct..
#include <string.h> // For memset.
#include <arpa/inet.h> // For inet_pton (), inet_ntop ().
#include <errno.h>
#include <stdbool.h>

#define SERVER_PORT 34568
#define BUFF_SIZE 1024

char bookingfile[BUFF_SIZE];
char bookingStatus[BUFF_SIZE];
char temp[BUFF_SIZE];

int main(int argc,char * argv[])
{
	remove("Booked.csv");
	if(argc != 2)
	{
		printf("Error in input\n");
		exit(0);
	}

	int TCPsockfd;
	int err;

	// Create socket

	TCPsockfd = socket(AF_INET,SOCK_STREAM,0);
	if(TCPsockfd == -1)
	{
		perror("Client: Socket Error");
		exit(0);
	}

	struct sockaddr_in srv_addr;

	socklen_t addrlen = sizeof (struct sockaddr_in); // size of the addresses.

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	memset(&(srv_addr.sin_zero), '\0', 8);

	err = inet_pton(AF_INET,argv[1],&srv_addr.sin_addr);
	if(err <= 0)
	{
		perror ("Client Presentation to network address conversion.\n");
        exit(0);
	}	

	// Connecting to Server
	err = connect(TCPsockfd,(struct sockaddr *)&srv_addr,addrlen);
	if(err == -1)
	{
		perror ("Client: Connect failed.");
	}

	FILE *fps = fopen("Booking.csv","r");
	FILE *fpr = fopen("Booked.csv","a");

	int i=1;
	while(fgets(bookingfile,BUFF_SIZE,fps) != NULL)
	{
		bookingStatus[0] = '\0';
		printf("\nExecuting %d request\n",i);
		bookingfile[strlen(bookingfile)-1] = '\0';
		// printf("%d) %s\n",i++,bookingfile);
		err = send(TCPsockfd,bookingfile,BUFF_SIZE,0);
		if(err == -1){
			perror("Error");
			exit(0);
		}

		char *token = strtok(bookingfile,",");
		int j=1;
		while(token != NULL)
		{
			if(j == 1 || j == 2 || j == 4)
			{
				strcat(bookingStatus,token);
				strcat(bookingStatus,",");
			}
			token = strtok(NULL,",");
			j++;
		}

		printf("Request sent to server\n");
		printf("Waiting for server response\n");
		sleep(5);

		err = recv(TCPsockfd,temp,BUFF_SIZE,0);
		if(err == -1){
			perror("Error");
			exit(0);
		}
		else if(err == 0)
		{
			perror("Connection Closed");
			exit(0);
		}
		printf("Received response fron the server\n");

		printf("Received message = %s\n",temp);

		strcat(bookingStatus,temp);

		char booktemp[BUFF_SIZE];
		sprintf(booktemp,"%s\n",bookingStatus);
		printf("Printing in file= %s\n",booktemp);
		fprintf(fpr,"%s",booktemp);
		i++;
	}

	fclose(fps);
	fclose(fpr);

	FILE *fpss = fopen("Booked.csv","r");

	printf("\nThe Booking Status :-\n");
	printf("\t--------------------------------------------------------------------------\n");
	printf("\tPassenger ID\t\tTrain\t#berths\t\tCoach(Berth)\n");
	printf("\t--------------------------------------------------------------------------\n");
	while(fgets(temp,BUFF_SIZE,fpss) != NULL)
	{	
		char *token = strtok(temp,",");
		char *psgID = strdup(token);
		char *allot;
		
		token = strtok(NULL,",");
		char *trainNo = strdup(token);

		token = strtok(NULL,",");
		char *berths = strdup(token);

		token = strtok(NULL,",");

		token[strlen(token)-1] = '\0';
		if(!strcmp(token,"NA"))
		{
			allot = strdup("SORRY! NO BERTHS AVAILABLE");
		}
		else
		{
			allot = strdup(token);
		}

		printf("\t%s\t\t\t%s\t%s\t\t%s\n",psgID,trainNo,berths,allot);
	}
	printf("\t--------------------------------------------------------------------------\n");
}