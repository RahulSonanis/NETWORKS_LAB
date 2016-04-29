/*
Reader File for Assignment-1

Name - Rahul Sonanis
Roll - 13CS10049
Name - Vishwas Jain
Roll - 13CS10053

*/

// Reader

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> // For the socket (), bind () etc. functions.
#include <netinet/in.h> // For IPv4 data struct..
#include <string.h> // For memset.
#include <arpa/inet.h> // For inet_pton (), inet_ntop ().
#include <errno.h>
#include <sys/wait.h>

#define SERVER_PORT 34567
#define BUFF_SIZE 1024

char date[BUFF_SIZE-1];
char headline[BUFF_SIZE];
char input[BUFF_SIZE];
char newsIndexSize[BUFF_SIZE];
char news[1000000];
char newsIndex[10000];
char dupnewsIndex[10000];

ssize_t readn(int , void *, size_t );

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

	// To indentify READER
	input[0]='1';
	input[1]='\0';
	err = send(TCPsockfd,input,BUFF_SIZE,0);
	if(err == -1)
		perror("error");

	while(1)
	{
		// Asking the reader which category to read
		printf("\n\n\tHI READER\n");
		printf("\nWhich of the following news group you want to read?\n");
		printf("\t1. Academic\n");
		printf("\t2. Non-Academic\n");
		printf("\t3. Exit\n");
		printf("Enter you choice: ");

		// Entering choice for reading category
		int choice;
		fgets(input,BUFF_SIZE,stdin);
		input[1] = '\0';

		if(input[0] == '1' || input[0] =='2')
		{
			if(input[0] == '1')					// Academic
			{
				err = send(TCPsockfd,input,BUFF_SIZE,0);
				if(err == -1){
					perror("Error");
					exit(0);
				}
			}
			else if(input[0] == '2'){			// Non Academic

				err = send(TCPsockfd,input,BUFF_SIZE,0);
				if(err == -1){
					perror("Error");
					exit(0);
				}
			}

			// Receiving the size of newsIndex of the selected category
			err = recv(TCPsockfd,newsIndexSize,BUFF_SIZE,0);
			if(err == -1){
				perror("Error");
				exit(0);
			}
			else if(err == 0)
			{
				perror("Connection Closed");
				exit(0);
			}

			// printf("\nnewsIndexSize = %d\n",atoi(newsIndexSize));

			// Receiving the newsIndex of the selected category
			err = readn(TCPsockfd,newsIndex,atoi(newsIndexSize));
			if(err == -1)
			{
				perror("Error");
				exit(0);
			}
			else if(err == 0)
			{
				perror("Connection Closed");
				exit(0);
			}
			// printf("News Index Size Received = %d\n",(int)strlen(newsIndex));

			if(strlen(newsIndex) == 0)
			{
				printf("\n\tNO DATA ON SERVER\n");
				continue;
			}
			else
			{
				// Printing the news index of the selected category
				strcpy(dupnewsIndex,newsIndex);
				int i=1;
			    printf("\nThe news index of the selected category:-\n");
			    printf("\n<=================================================================================================>\n\n");
			    char *token=strtok(newsIndex,"\n");

			    while(token != NULL)
			    {
			    	printf("%d) %s\n",i++,token);
			    	token = strtok(NULL,"\n");
			    }

			    printf("\n<=================================================================================================>\n\n");

			    // Sending the news serial which is to be read
			    printf("Select the serial of the news to be read\n => ");
			    while(1)
			    {
			    	fgets(input,BUFF_SIZE,stdin);
			    	if(atoi(input) < 0 || atoi(input) >= i)
			    	{
			    		continue;
			    	}
			    	else	break;
			    }
			    input[strlen(input)-1] = '\0';
			    err = send(TCPsockfd,input,BUFF_SIZE,0);
			    if(err == -1){
			    	perror("Error");
			    	exit(0);
			    }

			    // For Printing the date and headline of the news
			    token=strtok(dupnewsIndex,"\n");
			    i=1;
			    while(token != NULL)
			    {
			    	if(i == atoi(input))
			    	{
			    		sprintf(headline,"%s",token);
			    		break;
			    	}
			    	i++;
			    	token = strtok(NULL,"\n");
			    }


			    // Receiving the size of the news body 
			    err = recv(TCPsockfd,input,BUFF_SIZE,0);
			    if(err == -1)
			    {
			    	perror("Error");
			    	exit(0);
			    }
			    else if(err == 0)
				{
					perror("Connection Closed");
					exit(0);
				}

			    //printf("\nNews Body Size Received= %d\n",atoi(input));

			    // Receiving the news body
			    err = readn(TCPsockfd,news,atoi(input));
			    if(err == -1)
			    {
			    	perror("Error");
			    	exit(0);
			    }
			    else if(err == 0)
				{
					perror("Connection Closed");
					exit(0);
				}
			    //printf("News Size Received = %d\n",(int)strlen(news));


			    // Printing file in xterm
			    FILE *tmp = fopen("tmp.txt","w");
			    fprintf(tmp,"%s\n\n",headline);
			    fprintf(tmp,"%s",news);
			    fclose(tmp);

			    if(!fork())
			    {
			    	err = execlp("/usr/bin/xterm","/usr/bin/xterm","-hold","-e","more","tmp.txt",(const char*)NULL);
			    	//err = execlp("gedit","gedit","tmp.txt",(const char*)NULL);
					if(err == -1)
					{
						perror("Error in exec");
						exit(0);
					}

			    }
			    else
			    {
			    	wait(NULL);
			    }

				remove("tmp.txt");

				printf("\n\n");
			}
		}
		else if(input[0] == '3')
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

ssize_t readn(int fd, void *buffer, size_t n)
{
	ssize_t numRead; /* # of bytes fetched by last read() */
	size_t totRead; /* Total # of bytes read so far */
	char *buf;

	buf = buffer; /* No pointer arithmetic on "void *" */

	for (totRead = 0; totRead < n; ) {
		numRead = recv(fd, buf, n - totRead,0);

		if (numRead == 0) /* EOF */
		return totRead; /* May be 0 if this is first read() */

		if (numRead == -1) {
			if (errno == EINTR)
			continue; /* Interrupted --> restart read() */
			else
			return -1; /* Some other error */
		}

		totRead += numRead;
		buf += numRead;
	}
	return totRead; /* Must be 'n' bytes if we get here */
}