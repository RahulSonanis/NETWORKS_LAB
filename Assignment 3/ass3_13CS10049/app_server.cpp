/*  
	Application for testing server on top of 
	designed RTLP.
	
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


void exiting(int );
int sockfd;

int main(void)
{
	
	signal(SIGINT,exiting);
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("error:");
		exit(EXIT_FAILURE);
	}
	char recData[BUFF_SIZE];
	listenAccept(sockfd);
	printf("\n\tHandhake done!\n");
	int i = 0;
	for (; ;)
	{
		receive_data(sockfd, recData);
		printf("\nReceived Data - %s\n", recData);
		sprintf(recData, "ECHO RES %d", ++i);
		rtlp_send(sockfd, recData);
		printf("\nSent Data - %s\n", recData);
	}
	
	
	
	//close(sockfd);
	exit(EXIT_SUCCESS);
}


void exiting(int signum){
	if(close(sockfd) == -1){
		perror("Server encountered an Error while executing!\nExiting!");
		exit(0);
	}
	printf("Closing the server.\n");
	exit(0);
}
