/*
	Vishwas Jain      13CS10053
	Rahul Sonanis     13CS10049
	Assignment        4
	SMTP/POP  Client
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
#include <termios.h>
#include <signal.h>


#define BUFF_SIZE 1024

char buff[BUFF_SIZE];
char test[BUFF_SIZE];
char input[BUFF_SIZE];
struct sockaddr_in srv_addr;
int err,option,TCPsockfd;

int getPassword(char password[]);
int create_socket();
void connect_server(int ,char *,int );
ssize_t readn(int fd, void *buffer, size_t n);
void exit_client(int);

/*
   argv[1] = IP Address of the abc.com Server
   argv[2] = Domain (abc.com)
   argv[3] = SMTP_SERVER_PORT (abc.com)
   argv[4] = POP_SERVER_PORT  (abc.com)
   argv[5] = IP Address of the xyz.com Server
   argv[6] = Domain (xyz.com)
   argv[7] = SMTP_SERVER_PORT (xyz.com)
   argv[8] = POP_SERVER_PORT  (xyz.com)

 */

int main(int argc,char * argv[])
{
	signal(SIGINT, exit_client);

	if(argc != 5 && argc != 9)
	{
		printf("Error in input\n");
		printf("Usage: Server_IP_Address Domain_name SMTP-PORT POP-PORT [Other-Server-IP-Address Other_Domain SMTP-PORT POP-PORT] \n");
		exit(0);
	}

	if(argc == 9)
	{
		char argument[BUFF_SIZE];
        sprintf(argument,"./client %s %s %s %s",argv[5],argv[6],argv[7],argv[8]);
		if(!fork()){
			if(execlp("gnome-terminal","gnome-terminal","-e",argument,(const char*)NULL) == -1)
		      exit(0);
		}
	}

	
	int SMTP_SERVER_PORT = atoi(argv[3]); 
	int POP_SERVER_PORT = atoi(argv[4]);

	printf("%s Domain Client Started\n",argv[2]);

	while(1)
	{
		printf("\n>Do you want to (1) send emails OR (2) retrieve emails? OR (3) Quit\n>");
		fgets(input,BUFF_SIZE,stdin);
		input[strlen(input)-1] = '\0';
		option = atoi(input);

		if(option == 1) 			// SMTP 
		{
			TCPsockfd = create_socket();
			connect_server(TCPsockfd,argv[1],SMTP_SERVER_PORT);

			// SMTP connection initiation
			// waiting for server response
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("Error");
				exit(0);
			}

			// waiting for 220 service ready 
			strncpy(test,buff,3);
			test[3]='\0';
			if((strcmp(test,"220") < 0) || ((strcmp(test,"220") > 0)))
			{
				printf("Error in connection establishment\n");
				close(TCPsockfd);
				break;
			}

			// sending HELO command to server 
			sprintf(buff,"HELO %s",argv[2]);
			err = send(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1)
			{
				perror("sending error");
				exit(0);
			}

			// waiting for server response to HELO
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("receiving Error");
				exit(0);
			}

			// 421 <domain name> service not available checking error
			strncpy(test,buff,3);
			test[3]='\0';
			if(!strcmp(test,"421"))
			{
				printf("Error in sending mail: %s\n",buff);
				close(TCPsockfd);
				break;
			}

			// SMTP connection established
			printf("\nSMTP connection established with server\n");

			while(1)
			{
				printf("\n>Do you want to (1)send email OR (2)verify user name OR (3)quit?\n>");
				fgets(input,BUFF_SIZE,stdin);
				input[strlen(input)-1] = '\0';
				option = atoi(input);

				if(option == 1)				// sending email
				{
					int next = 0;
					// MAIL FROM part
					while(1)
					{
						// taking sender's username from the user
						while(1)
						{
							printf(">Mail from?\n>");
							fgets(input,BUFF_SIZE,stdin);
							input[strlen(input)-1] = '\0';

							if(strlen(input) == 0)
							{
								printf("\nINCORRECT INPUT!! Please try again!!\n");
								continue;
							}
							else 	break;
						}


						// now sending MAIL FROM command with username of sender to server
						sprintf(buff,"MAIL FROM: %s",input);
						err = send(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						// waiting for server response to MAIL FROM  (no error checking as already checked in VRFY)
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}

						// error checking for MAIL FROM
						strncpy(test,buff,3);
						test[3]='\0';
						if(!strcmp(test,"550") || !strcmp(test,"501"))
						{
							printf("%s\n",buff);
							
							printf(">Do you want to continue email? (1)Yes (2)No\n>");
							fgets(input,BUFF_SIZE,stdin);
							input[strlen(input)-1] = '\0';
							option = atoi(input);

							if(option == 2)
							{
								next = 1;
								break;
							}
							else
							{
								continue;
							}

						}
						else if(!strcmp(test,"250"))
						{
							break;
						}
					}
					
					if(next)
					{
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET",BUFF_SIZE,0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}
						// waiting for server response to QUIT
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}
						continue;		// user does not want to send email
					}	

					next = 0;

					// RCPT TO part
					while(1)
					{
						// taking receiver's username from the user
						while(1)
						{
							printf(">Mail to?\n>");
							fgets(input,BUFF_SIZE,stdin);
							input[strlen(input)-1] = '\0';

							if(strlen(input) == 0)
							{
								printf("\nINCORRECT INPUT!! Please try again!!\n");
								continue;
							}
							else 	break;
						}

						// now sending RCPT TO command with username of sender to server
						sprintf(buff,"RCPT TO: %s",input);
						err = send(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						// waiting for server response to RCPT TO
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}

						// error checking for RCPT TO
						strncpy(test,buff,3);
						test[3]='\0';
						if(!strcmp(test,"501") || !strcmp(test,"550"))
						{
							printf("%s\n",buff);

							printf(">Do you want to continue email? (1)Yes (2)No\n>");
							fgets(input,BUFF_SIZE,stdin);
							input[strlen(input)-1] = '\0';
							option = atoi(input);


							if(option == 2)
							{
								next = 1;
								break;
							}
							else
							{
								continue;
							}
						}
						else if(!strcmp(test,"250"))
						{
							break;
						}
					}


					if(next)
					{
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET",BUFF_SIZE,0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}
						// waiting for server response to QUIT
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}
						continue;		// user does not want to send email
					}	



					// now sending DATA command to server
					err = send(TCPsockfd,"DATA",BUFF_SIZE,0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}
					// waiting for server response to DATA
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}
					
					// error checking for DATA
					strncpy(test,buff,3);
					test[3]='\0';
					if(!strcmp(test,"451"))
					{
						printf("%s\n",buff);
						continue;
					}
					else if(!strcmp(test,"354"))
					{
						printf("%s\n",buff);
					}

					// start of data
					printf("\n>");

					while(1)
					{
						fgets(input,BUFF_SIZE,stdin);
						sprintf(buff,"%s",input);
						// printf("%s\n",input);

						err = send(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						input[strlen(input)-1] = '\0';

						if(!strcmp(input,"."))
						{
							break;
						}

						printf(">");
					}

					// waiting for server response to end of DATA
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}
					
					// error checking for DATA
					strncpy(test,buff,3);
					test[3]='\0';
					if(!strcmp(test,"250"))
					{
						printf("Mail Sent Successfully\n");
					}
					continue;
				}	
				else if(option == 2)	// verify user name
				{	
					// taking sender's username from the user
					while(1)
					{
						printf("\n>Enter Name of the User to verify:\n>");
						fgets(input,BUFF_SIZE,stdin);
						input[strlen(input)-1] = '\0';

						if(strlen(input) == 0)
						{
							printf("\nINCORRECT INPUT!! Please try again!!\n");
							continue;
						}
						else 	break;
					}

					// checking with VRFY command whether this username exists or not on the server
					sprintf(buff,"VRFY %s",input);
					err = send(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					// waiting for server response to VRFY
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// error checking for VRFY
					strncpy(test,buff,3);
					test[3]='\0';
					if(!strcmp(test,"553") || !strcmp(test,"550"))
					{
						printf("%s\n",buff);
					}
					else if(!strcmp(test,"250"))
					{
						printf("%s\n",buff);
					}
					continue;
				}
				else if(option == 3)
				{					
					// now sending QUIT command to server
					err = send(TCPsockfd,"QUIT",BUFF_SIZE,0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}
					// waiting for server response to QUIT
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}
					printf("%s\n",buff);
					close(TCPsockfd);
					break;
				}
				else
				{
					printf("Please enter correct option\n");
					continue;
				}
			}
		}
		else if(option == 2)    // POP
		{
			TCPsockfd = create_socket();
			connect_server(TCPsockfd,argv[1],POP_SERVER_PORT);

			// POP connection initiation
			// waiting for server response
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("Error");
				exit(0);
			}

			// waiting for +OK ready 
			strncpy(test,buff,3);
			test[3]='\0';
			if((strcmp(test,"+OK") < 0) || ((strcmp(test,"+OK") > 0)))
			{
				printf("Error in connection establishment\n");
				break;
			}

			int next = 0;

			while(1)
			{
				// taking username from the user
				while(1)
				{
					printf("\n>Username?\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';

					if(strlen(input) == 0)
					{
						printf("\nINCORRECT INPUT!! Please try again!!\n");
						continue;
					}
					else 	break;
				}

				// now sending USER command
				sprintf(buff,"USER %s",input);
				err = send(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}

				// waiting for server response to USER
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}

				// error checking for USER
				strncpy(test,buff,3);
				test[3]='\0';
				if(!strcmp(test,"-ER"))
				{
					printf("%s\n",buff);

					printf(">Do you want to continue? (1)Yes (2)No\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';
					option = atoi(input);

					if(option == 2)
					{
						next = 1;
						break;
					}
					else
					{
						continue;
					}
				}
				else
				{
					break;
				}
			}
			
			if(next)				// user does not wish to continue
			{
				// now sending QUIT command to server
				err = send(TCPsockfd,"QUIT",BUFF_SIZE,0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}
				// waiting for server response to QUIT
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}
				printf("%s\n",buff);
				close(TCPsockfd);
				continue;
			}
			
			next = 0;

			while(1)
			{
				// taking password from the user
				while(1)
				{
					printf(">Password?\n>");
					int hi = getPassword(input);

					if(hi == 0)
					{
						printf("\nINCORRECT INPUT!! Please try again!!\n");
						continue;
					}
					else 	break;
				}

				// now sending PASS command
				sprintf(buff,"PASS %s",input);
				err = send(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}

				// waiting for server response to PASS
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}

				// error checking for PASS
				strncpy(test,buff,3);
				test[3]='\0';
				if(!strcmp(test,"-ER"))
				{
					printf("%s\n",buff);

					printf("\n>Do you want to continue? (1)Yes (2)No\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';
					option = atoi(input);

					if(option == 2)
					{
						next = 1;
						break;
					}
					else
					{
						continue;
					}
				}
				else
				{
					break;
				}
			}

			if(next)			// user does not wish to continue
			{
				// now sending QUIT command to server
				err = send(TCPsockfd,"QUIT",BUFF_SIZE,0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}
				// waiting for server response to QUIT
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}
				printf("%s\n",buff);
				close(TCPsockfd);
				continue;
			}

			// now sending STAT command
			sprintf(buff,"STAT");
			err = send(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1)
			{
				perror("sending error");
				exit(0);
			}

			// waiting for server response to STAT
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("receiving Error");
				exit(0);
			}

			// error checking for STAT
			strncpy(test,buff,3);
			test[3]='\0';
			if(!strcmp(test,"+OK"))
			{
				printf(">%s\n",&buff[4]);
			}
			
			
			// LIST COMMAND
			sprintf(buff,"LIST");
			err = send(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1)
			{
				perror("sending error");
				exit(0);
			}

			// waiting for server response to LIST
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("receiving Error");
				exit(0);
			}


			// error checking for LIST
			strncpy(test,buff,3);
			test[3]='\0';
			if(!strcmp(test,"+OK"))		{}

			int i=0;

			while(1)
			{
				err = readn(TCPsockfd,buff,BUFF_SIZE);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}

				buff[strlen(buff)-1] = '\0';

				if(!strcmp(buff,"."))
				{
					break;
				}

				printf("%s\n",buff);
				i++;
			}

			while(1)
			{
				if(i > 0)
				{
					printf("\n>Which one do you want to read?\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';
					
					sprintf(buff,"RETR %s",input);
					err = send(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					// waiting for server response to RETR
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// error checking for RETR
					strncpy(test,buff,3);
					test[3]='\0';
					if(!strcmp(test,"+OK"))
					{
						printf("%s\n",buff);

						while(1)
						{
							err = readn(TCPsockfd,buff,BUFF_SIZE);
							if(err == -1){
								perror("receiving Error");
								exit(0);
							}
							buff[strlen(buff)-1] = '\0';

							if(!strcmp(buff,"."))
							{
								break;
							}

							printf("\t%s\n",buff);
						}
					}
					else if(!strcmp(test,"-ER"))
					{
						printf("%s\n",buff);
					}

					printf("\n>Do you want to read more? (1)Yes (2)No\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';
					option = atoi(input);

					if(option == 2)
					{
						break;
					}
					else
					{
						continue;
					}
				}
				else
				{
					break;
				}
			}

			// now sending QUIT command to server
			err = send(TCPsockfd,"QUIT",BUFF_SIZE,0);
			if(err == -1)
			{
				perror("sending error");
				exit(0);
			}
			// waiting for server response to QUIT
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("receiving Error");
				exit(0);
			}
			printf("%s\n",buff);
			close(TCPsockfd);
			continue;

		}
		else if(option == 3)	// Quit
		{
			break;
		}
		else
		{
			printf("Please enter correct option\n");
			continue;
		}
	}
	//exit(0);
}



void exit_client(int signum){
      close(TCPsockfd);
      //printf("\nQuiting the server.\n");
      exit(0);
}



int getPassword(char password[])
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

    return (int)strlen(password);
}



int create_socket()
{
	// Create socket
	int TCPsoc = socket(AF_INET,SOCK_STREAM,0);
	if(TCPsoc == -1)
	{
		perror("Client: Socket Error");
		exit(0);
	}
	return TCPsoc;
}



void connect_server(int TCPsoc,char *arg1,int port)
{
	socklen_t addrlen = sizeof (struct sockaddr_in); // size of the addresses.

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	memset(&(srv_addr.sin_zero), '\0', 8);

	int err = inet_pton(AF_INET,arg1,&srv_addr.sin_addr);
	if(err <= 0)
	{
		perror ("Client Presentation to network address conversion.\n");
        exit(0);
	}

	// Connecting to Server
	err = connect(TCPsoc,(struct sockaddr *)&srv_addr,addrlen);
	if(err == -1)
	{
		perror ("Client: Connect failed.");
        exit(0);
	}
}



ssize_t readn(int fd, void *buffer, size_t n)
{
	ssize_t numRead; /* # of bytes fetched by last read() */
	size_t totRead; /* Total # of bytes read so far */
	char *buf;

	buf = (char *)buffer; /* No pointer arithmetic on "void *" */

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