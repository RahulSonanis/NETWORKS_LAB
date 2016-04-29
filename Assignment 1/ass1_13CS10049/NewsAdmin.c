/*
Admin File for Assignment-1

Name - Rahul Sonanis
Roll - 13CS10049
Name - Vishwas Jain
Roll - 13CS10053

*/


// Admin

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> // For the socket (), bind () etc. functions.
#include <netinet/in.h> // For IPv4 data struct..
#include <string.h> // For memset.
#include <arpa/inet.h> // For inet_pton (), inet_ntop ().
#include <stdbool.h>
#include <termios.h>

#define SERVER_PORT 23456
#define BUFF_SIZE 1024

char date[BUFF_SIZE-1];
char headline[BUFF_SIZE];
char datehead[BUFF_SIZE];
char input[BUFF_SIZE];

// ssize_t readn(int , void *, size_t );
bool datecheck();
void getPassword(char password[]);

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

	TCPsockfd = socket(AF_INET,SOCK_DGRAM,0);

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

	// // To identify ADMIN
	// input[0]='3';
	// input[1]='\0';
	// err = sendto(TCPsockfd,input,BUFF_SIZE,0,(struct sockaddr *)&srv_addr,addrlen);
	// if(err == -1)
	// 	perror("error");
	// printf("\nSize of data sent=> %d\n",err);



	// Asking admin for password
	printf("\tHI ADMIN\n");
	printf("Enter your password=> ");
	while(1){
		
		getPassword(input);

		err = sendto(TCPsockfd,input,BUFF_SIZE,0,(struct sockaddr *)&srv_addr,addrlen);
		if(err == -1)
			perror("error");
		// printf("\nSize of password sent=> %d\n",(int)strlen(input));

		struct sockaddr_in sender_addr;
		socklen_t sender_len;
		err = recvfrom(TCPsockfd,input,BUFF_SIZE,0,(struct sockaddr *)&sender_addr,&sender_len);
		if(err == -1)
		{
			perror("error");
			exit(0);
		}
		else if(err == 0)
		{
			perror("Connection Closed");
			exit(0);
		}

		if(!strcmp(input,"0"))
		{
			printf("Wrong Password\nPlease Enter Again=> ");
			continue;
		}
		else if(!strcmp(input,"1"))
		{
			printf("Correct Password\n");
			break;
		}
		else	break;
	}

	// Taking input DATE
	printf("\nEnter the date (DD-MM-YYYY) after which the news should be deleted\n");
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

	printf("\nSending Date => %s\n",datehead);
	err = sendto(TCPsockfd,datehead,BUFF_SIZE,0,(struct sockaddr *)&srv_addr,addrlen);
		if(err == -1)
			perror("error");
	// printf("\nLength of password sent=> %d\n\n",(int)strlen(datehead));
	printf("\tTHANK YOU\n");

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

// ssize_t readn(int fd, void *buffer, size_t n)
// {
// 	ssize_t numRead; /* # of bytes fetched by last read() */
// 	size_t totRead; /* Total # of bytes read so far */
// 	char *buf;

// 	buf = buffer; /* No pointer arithmetic on "void *" */

// 	for (totRead = 0; totRead < n; ) {
// 		numRead = recv(fd, buf, n - totRead,0);

// 		if (numRead == 0) /* EOF */
// 		return totRead; /* May be 0 if this is first read() */

// 		if (numRead == -1) {
// 			if (errno == EINTR)
// 			continue; /* Interrupted --> restart read() */
// 			else
// 			return -1; /* Some other error */
// 		}

// 		totRead += numRead;
// 		buf += numRead;
// 	}
// 	return totRead; /* Must be 'n' bytes if we get here */
// }


void getPassword(char password[])
{
    static struct termios oldt, newt;
    int i = 0;
    int c;

    /*saving the old settings of STDIN_FILENO and copy settings for resetting*/
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;

    /*setting the approriate bit in the termios struct*/
    newt.c_lflag &= ~(ECHO);          

    /*setting the new bits*/
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    /*reading the password from the console*/
    while ((c = getchar())!= '\n' && c != EOF && i < BUFF_SIZE){
        password[i++] = c;
    }
    password[i] = '\0';

    /*resetting our old STDIN_FILENO*/ 
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

}