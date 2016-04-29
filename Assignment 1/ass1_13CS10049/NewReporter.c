/*
Reporter File for Assignment-1

Name - Rahul Sonanis
Roll - 13CS10049
Name - Vishwas Jain
Roll - 13CS10053

*/

// Reporter

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

#define SERVER_PORT 34567
#define BUFF_SIZE 1024

char date[BUFF_SIZE-1];
char headline[BUFF_SIZE];
char datehead[BUFF_SIZE];
char input[BUFF_SIZE];
char news[1000000];

ssize_t writen(int , const void *, size_t );
bool datecheck();

int main(int argc,char * argv[])
{
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
        exit(0);
	}

	// To identify REPORTER
	input[0]='2';
	input[1]='\0';
	err = send(TCPsockfd,input,BUFF_SIZE,0);
	if(err == -1)
		perror("first");

	while(1)
	{
		printf("\n\t\tHI REPORTER\n");
		printf("Which of the following two news group you want to contribute?\n");
		printf("\t1. Academic\n");
		printf("\t2. Non-Academic\n");
		printf("\t3. Exit\n");
		printf("Enter you choice: ");

		int choice;
		fgets(input,BUFF_SIZE,stdin);
		input[1] = '\0';

		if(input[0] == '1' || input[0] =='2')
		{
			if(input[0] == '1')		// Academic
			{
				err = send(TCPsockfd,input,BUFF_SIZE,0);
				if(err == -1){
					perror("Error");
					exit(0);
				}

			}
			else if(input[0] == '2'){	// Non-Academic
				err = send(TCPsockfd,input,BUFF_SIZE,0);
				if(err == -1){
					perror("Error");
					exit(0);
				}

			}

			// Taking input DATE
			printf("\nEnter the date (DD-MM-YYYY) for which news is to be submitted\n");
			fgets(date,BUFF_SIZE,stdin);
			date[strlen(date)-1] ='\0';

			strcpy(datehead,date);
			// Checking Date
			while(!datecheck())
			{
				printf("\nWrong Date Entered. FORMAT => (DD-MM-YYYY)\nEnter Again=> ");
				fgets(date,BUFF_SIZE,stdin);
				date[strlen(date)-1] ='\0';
				strcpy(datehead,date);
			}

			// Taking input HEADLINE
			printf("\nEnter the Headline of the news\n");
			fgets(headline,BUFF_SIZE,stdin);

			while(1){
				if(strlen(headline) >= 240)
				{
					printf("\nHeadline too long\nEnter Again=> ");
					fgets(headline,BUFF_SIZE,stdin);
				}
				else	break;
			}

			headline[strlen(headline)-1]='\0';

			// Sending date and headline to server
			strcat(datehead," ");
			strcat(datehead,headline);
			//printf("Date and Headline =%s\n",datehead);
			err = send(TCPsockfd,datehead,BUFF_SIZE,0);
			if(err == -1){
				perror("Error");
				exit(0);
			}

			//Taking input news body file name
			printf("\nEnter the name of file(with extension) which is body of the news to send to server\n=> ");
			fgets(input,BUFF_SIZE,stdin);
			input[strlen(input)-1] = '\0';
			FILE *fp = fopen(input,"r");

			while(1)
			{
				if(fp == NULL)
				{
					printf("\nFile Name Not Valid Try Again\n=> ");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';
					fp = fopen(input,"r");
				}
				else break;
			}

			// Scanning the news body
			news[0]='\0';			
			while(fgets(input, sizeof(input), fp) != NULL) 
		    {
		      	strcat(news,input);
		    }
		    fclose(fp);

		    // Printing the body of the news
		    printf("\nThe body of the news:-\n");
		    printf("\n<=================================================================================================>\n");
		    printf("\n%s\n",news);
		    printf("\n<=================================================================================================>\n\n");

		    // Sending the size of the body of the news to the server
		    int len=strlen(news)+1;
		    sprintf(input,"%d",len);
		    //printf("size of body=%s\n",input);
		   	err = send(TCPsockfd,input,BUFF_SIZE,0);
		   	if(err == -1){
		   		perror("Error");
				exit(0);
		   	}

		   	// Sending the body of the news
		   	int siz = writen(TCPsockfd,news,strlen(news)+1);
		   	if(siz == -1)
		   	{
		   		perror("Error");
				exit(0);
		   	}
		   	//printf("SIZE OF NEWS BODY SENT=%d\n",siz);

		   	printf("\n\n");

		}
		else if(input[0] == '3')	// exit
		{
			err = send(TCPsockfd,"3",BUFF_SIZE,0);

			if(err == -1)
			{
				perror("Error");
				exit(0);
			}
			close(TCPsockfd);
			exit(0);
		}
	}
}


bool datecheck()
{
	bool leap;
	int dmonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	char * token = strtok(date,"-");
	// printf("dattok = %s",token);
	if(strlen(token) != 2)
	{
		return false;
	}
	int date1 = atoi(token);

	token = strtok(&date[3],"-");
	// printf("montok = %s",token);
	if(strlen(token) != 2)
	{
		return false;
	}
	int month = atoi(token);

	token = strtok(&date[6],"\n");
	// printf("yeartok = %s",token);
	if(strlen(token) != 4)
	{
		return false;
	}
	int year = atoi(token);

	if(year%400 == 0)
	{
		leap = true;
	}
	else if(year%100 == 0)
	{
		leap=false;
	}
	else if(year%4 == 0)
	{
		leap = true;
	}
	else
	{
		leap = false;
	}

	if(month < 0 || month > 12)
	{
		return false;
	}

	if(month == 2 && leap)
	{
		dmonth[2] = 29;
	}

	if(date1 < 0 || date1 > dmonth[month-1])
	{
		return false;
	}
	return true;
}


ssize_t writen(int fd, const void *buffer, size_t n)
{
	ssize_t numWritten; /* # of bytes written by last write() */
	size_t totWritten; /* Total # of bytes written so far */
	const char *buf;
	buf = buffer; /* No pointer arithmetic on "void *" */

	for (totWritten = 0; totWritten < n; ) {

		numWritten = send(fd, buf, n - totWritten,0);
		if (numWritten <= 0) {
				 if (numWritten == -1 && errno == EINTR)
				 continue; /* Interrupted --> restart write() */
				 else
				 return -1; /* Some other error */
		}

	totWritten += numWritten;
	buf += numWritten;
	}
	
	return totWritten; /* Must be 'n' bytes if we get here */
}