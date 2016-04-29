/*
	Computer Networks Laboratory
	Assignment - 3 (Application Implementation on top of RTLP Protocol)
	Client side Code
	Rahul Sonanis	13CS10049
	Vishwas Jain	13CS10053
*/

#include "RTLP_client.h"

int socks;

void exiting(int signum){
	if(close(socks) == -1){
		perror("Server encountered an Error while executing!\nExiting!");
		exit(0);
	}
	printf("Closing the server.\n");
	exit(0);
}

int main(int argc, char * argv[])
{
	if(argc != 3)
	{
		printf("Error in Input\n");
		exit(0);
	}
	srand(time(NULL));

	signal(SIGINT,exiting);

	int socks,er;

	if((socks = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) 
	{
		perror("error in creating raw socket:");
		exit(EXIT_FAILURE);
	}

    er = rtlp_connect(socks,argv[1],argv[2]);
    if(er == 0)
    {
    	printf("Error in three way handshaking\n");
    }
    else
    {
    	printf("Three Way Handshaking Done\n");
    }

    printf("\nSending Data to server from main\n");

    char hello[BUFF_SIZE];
    char receiving[BUFF_SIZE];

    for(int i = 0 ; i < 15 ; i++)
    {
    	sprintf(hello,"ECHO REQ %d",i+1);
    	er = rtlp_send(socks,hello);
	    if(er == 0)
	    {
	    	printf("Error in sending\n");
	    }
	    else
	    {
	    	printf("Sending Successful\n");
	    }
	    printf("Sent Data to the server = %s\n",hello);
	    receive_data(socks,receiving);
    	printf("Received data from server = %s\n\n",receiving);
    }

    er = rtlp_terminate(socks);

    if(er == 0)
    {
    	printf("Error in terminating connection\n");
    }
    else
    {
    	printf("\nConnection closed Successfully with three way handshaking\n");
    }

	exit(EXIT_SUCCESS);
}
