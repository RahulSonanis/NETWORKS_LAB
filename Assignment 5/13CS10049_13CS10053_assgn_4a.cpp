/*
	Vishwas Jain      13CS10053
	Rahul Sonanis     13CS10049
	Assignment        4A
	Standard SMTP/POP Client
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


#define BUFF_SIZE 4096

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
*/

int main(int argc,char * argv[])
{
	signal(SIGINT, exit_client);

	if(argc != 5)
	{
		printf("Error in input\n");
		printf("Usage: Server-IP-Address Your_Domain SMTP-PORT POP-PORT \n");
		exit(0);
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
			memset(buff, '\0', sizeof(buff));
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

				printf("Error in connection establishment, server replied with:\n%s\n",buff);
				close(TCPsockfd);
				break;
			}


			// sending HELO command to server
			sprintf(buff,"HELO %s\r\n",argv[2]);
			err = send(TCPsockfd,buff,strlen(buff),0);
			if(err == -1)
			{
				perror("sending error");
				exit(0);
			}

			memset(buff, '\0', sizeof(buff));
			// waiting for server response to HELO
			err = recv(TCPsockfd,buff,BUFF_SIZE,0);
			if(err == -1){
				perror("receiving Error");
				exit(0);
			}

			// 421 <domain name> service not available checking error
			strncpy(test,buff,3);
			test[3]='\0';
			if(strcmp(test,"250"))
			{
				printf("Error in sending mail: %s\n",buff);
				// now sending QUIT command to server
				err = send(TCPsockfd,"QUIT\r\n",strlen("QUIT\r\n"),0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}
				memset(buff, 0, sizeof(buff));
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

			// SMTP connection established
			printf("\nSMTP connection established with server\n");

			while(1)
			{
				printf("\n>Do you want to (1)send email OR (2)quit?\n>");
				fgets(input,BUFF_SIZE,stdin);
				input[strlen(input)-1] = '\0';
				option = atoi(input);

				if(option == 1)				// sending email
				{
					int next = 0;
					// MAIL FROM part

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
					sprintf(buff,"MAIL FROM: <%s>\r\n",input);
					err = send(TCPsockfd,buff,strlen(buff),0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					memset(buff, 0, sizeof(buff));
					// waiting for server response to MAIL FROM  (no error checking as already checked in VRFY)
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// error checking for MAIL FROM
					strncpy(test,buff,3);
					test[3]='\0';

					if(strcmp(test,"250") != 0)
					{
						printf("%s\n",buff);
						next =1;
					}


					if(next)
					{
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET\r\n",strlen("RSET\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						memset(buff, 0, sizeof(buff));
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
					sprintf(buff,"RCPT TO: <%s>\r\n",input);
					err = send(TCPsockfd,buff,strlen(buff),0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					memset(buff, 0, sizeof(buff));
					// waiting for server response to RCPT TO
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// error checking for RCPT TO
					strncpy(test,buff,3);
					test[3]='\0';

					if((strcmp(test,"250") != 0) && (strcmp(test,"251") != 0))
					{
						printf("%s\n",buff);
						next =1 ;
					}


					if(next)
					{
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET\r\n",strlen("RSET\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						memset(buff, 0, sizeof(buff));
						// waiting for server response to QUIT
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}
						continue;		// user does not want to send email
					}

					next = 0;

					// now sending DATA command to server
					err = send(TCPsockfd,"DATA\r\n",strlen("DATA\r\n"),0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					memset(buff, 0, sizeof(buff));
					// waiting for server response to DATA
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// error checking for DATA
					strncpy(test,buff,3);
					test[3]='\0';

					if(!strcmp(test,"354"))
					{
						printf("%s",buff);
					}
					else{
						printf("%s\n",buff);
						next = 1;
					}

					if(next)
					{
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET\r\n",strlen("RSET\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						memset(buff, 0, sizeof(buff));
						// waiting for server response to QUIT
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}
						continue;		// user does not want to send email
					}

					// start of data
					printf("\n>");

					while(1)
					{

						fgets(input,BUFF_SIZE,stdin);
						//
						input[strlen(input)-1]='\0';
						//

						sprintf(buff,"%s\r\n",input);

						err = send(TCPsockfd,buff,strlen(buff),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						if(!strcmp(input,"."))
						{
							break;
						}

						printf(">");
					}

					memset(buff, 0, sizeof(buff));
					// waiting for server response to end of DATA
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					printf("After sending mail data rec- %s\n", buff);
					// error checking for DATA
					strncpy(test,buff,3);
					test[3]='\0';
					if(!strcmp(test,"250"))
					{
						printf("Mail Sent Successfully\n");
					}
					else{
						printf("%s\n",buff);
						// now sending RSET command to server
						err = send(TCPsockfd,"RSET\r\n",strlen("RSET\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}

						memset(buff, 0, sizeof(buff));
						// waiting for server response to QUIT
						err = recv(TCPsockfd,buff,BUFF_SIZE,0);
						if(err == -1){
							perror("receiving Error");
							exit(0);
						}
					}
					continue;
				}
				else if(option == 2)
				{
					// now sending QUIT command to server
					err = send(TCPsockfd,"QUIT\r\n",strlen("QUIT\r\n"),0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}

					memset(buff, 0, sizeof(buff));
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
			memset(buff, 0, sizeof(buff));
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
				sprintf(buff,"USER %s\r\n",input);
				err = send(TCPsockfd,buff,strlen(buff),0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}

				memset(buff, 0, sizeof(buff));
				// waiting for server response to USER
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}
				printf("%s\n",buff );

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
						// now sending QUIT command to server
						err = send(TCPsockfd,"QUIT\r\n",strlen("QUIT\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}
						memset(buff, 0, sizeof(buff));
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
						continue;
					}
				}

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
				sprintf(buff,"PASS %s\r\n",input);
				err = send(TCPsockfd,buff,strlen(buff),0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}

				memset(buff, 0, sizeof(buff));
				// waiting for server response to PASS
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}
				printf("%s\n",buff );

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
						err = send(TCPsockfd,"QUIT\r\n",strlen("QUIT\r\n"),0);
						if(err == -1)
						{
							perror("sending error");
							exit(0);
						}
						memset(buff, 0, sizeof(buff));
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
						continue;
					}
				}

				// LIST COMMAND
				sprintf(buff,"LIST\r\n");
				err = send(TCPsockfd,buff,strlen(buff),0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}

				memset(buff, 0, sizeof(buff));
				// waiting for server response to LIST
				err = recv(TCPsockfd,buff,BUFF_SIZE,0);
				if(err == -1){
					perror("receiving Error");
					exit(0);
				}

				// error checking for LIST
				strncpy(test,buff,3);
				test[3]='\0';
				if(!strcmp(test,"+OK"))
				{
					// printf("\n\n\tMessage No.\tSize\n");
					// char *token = strtok(&buff[5]," ");

					// while(token != NULL)
					// {
					// 	printf("\t%s",token);
					// 	token = strtok(NULL,"\r\n");	/////////
					// 	printf("\t\t%s\n",token);
					// 	token = strtok(NULL," ");
					// }
					printf("%s",&buff[4]);
				}

				while(1)
				{

					printf("\n>Which one do you want to read?\n>");
					fgets(input,BUFF_SIZE,stdin);
					input[strlen(input)-1] = '\0';

					sprintf(buff,"RETR %s\r\n",input);
					err = send(TCPsockfd,buff,strlen(buff),0);
					if(err == -1)
					{
						perror("sending error");
						exit(0);
					}


					memset(buff, 0, sizeof(buff));
					// waiting for server response to RETR
					err = recv(TCPsockfd,buff,BUFF_SIZE,0);
					if(err == -1){
						perror("receiving Error");
						exit(0);
					}

					// printf("%s\n", );
					printf("\n");
					int len = 0;
					while(len <= err)
					{
						printf("%c",buff[len]);
						len++;
					}
					printf("\n");
					
					// // error checking for RETR
					// strncpy(test,buff,3);
					// test[3]='\0';
					// if(!strcmp(test,"+OK"))
					// {
					// 	printf("%s",buff);

					// }
					// else if(!strcmp(test,"-ER"))
					// {
					// 	printf("%s",buff);
					// }

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

				// now sending QUIT command to server
				err = send(TCPsockfd,"QUIT\r\n",strlen("QUIT\r\n"),0);
				if(err == -1)
				{
					perror("sending error");
					exit(0);
				}
				memset(buff, 0, sizeof(buff));
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
