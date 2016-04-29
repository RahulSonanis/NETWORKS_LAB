/*
Server File for Assignment-1

Name - Rahul Sonanis
Roll - 13CS10049
Name - Vishwas Jain
Roll - 13CS10053
*/



#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <stdbool.h>
#define TCP_SERVER_PORT 34567
#define UDP_SERVER_PORT 23456
#define BUFF_SIZE 1024
#define BACKLOG 20


char acadFilePath[300];								// Path of the "Academic.txt" file
char nonacadFilePath[300];								// Path of the "NonAcademic.txt" file
char acadFolderPath[300];								// Path of the "Academic.txt" file
char nonacadFolderPath[300];								// Path of the "NonAcademic.txt" file
ssize_t readn(int , void *, size_t );
ssize_t writen(int , const void *, size_t);
void writeHeading(char* , char* );
bool dateCompare(char* , char*);
void deletFiles(char* );
int main(int argc, char* argv[]) {

    if(argc != 2){
        printf("Usage ./server IP-Address\n");
        exit(0);
    }
	mkdir("Academic",ACCESSPERMS);
	mkdir("Non-Academic",ACCESSPERMS);
	sprintf(acadFolderPath, "%s/Academic", getcwd(NULL,0));
	sprintf(nonacadFolderPath, "%s/Non-Academic", getcwd(NULL,0));
	sprintf(acadFilePath, "%s/Academic.txt",acadFolderPath);
	sprintf(nonacadFilePath, "%s/NonAcademic.txt",nonacadFolderPath);

	if(!fork()) {
		int sin_size, TCPsockfd, new_fd;
		struct sockaddr_in server_addr, client_addr;
		TCPsockfd = socket(AF_INET, SOCK_STREAM, 0);
		int yes=1;
		if(TCPsockfd == -1){
			perror("Error in creating Socket!");
			exit(0);
		}
		// lose the pesky "Address already in use" error message
		if (setsockopt(TCPsockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
		}
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(TCP_SERVER_PORT);
		if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0){
			perror("Error in creating server.");
		}
		//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		memset(&(server_addr.sin_zero), '\0', 8);

		if(bind(TCPsockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
			perror("Error in Binding");
			exit(0);
		}

		if(listen(TCPsockfd, BACKLOG) == -1){
			perror("listen");
			exit(0);
		}
		printf("TCP\n");
		while(1)
		{

			sin_size = sizeof(struct sockaddr_in);
			new_fd = accept(TCPsockfd, (struct sockaddr *)&client_addr, &sin_size);
			printf("TCP connected\n");
			if(new_fd == -1){
				perror("Server Accept Failed");
				continue;
			}

			if(!fork()){
				close(TCPsockfd);
				char childbuff[1023];
				char news[1000000];
				char newsFilePath[300];
				int recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);
				if(recErr == -1){
					perror("Connection lost1");
					exit(0);
				}
				else if(recErr == 0){
					perror("Connection lost2");
					exit(0);
				}

				if(childbuff[0] =='1'){
					char newsFiles[50][100000];
					printf("Connection established with some Reader\n");
					while(1){
						recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);

						if(recErr == -1){
							perror("Connection closed from Reader");
							exit(0);
						}
						else if(recErr == 0){
							perror("Connection closed from Reader");
							exit(0);
						}

						if(childbuff[0] == '1'){	
							printf("Inside academic\n");
							
							FILE* acadFile = fopen(acadFilePath,"a+");
							if(acadFile == NULL){
								perror("Error in opeing files");
								exit(0);
							}
							if(flock(fileno(acadFile), LOCK_EX) == -1){
								perror("Error in accessing files");
								exit(0);
							}
							int fileCount =0 ;
							news[0] = '\0';
							while(fgets(childbuff, sizeof(childbuff), acadFile) != NULL){

								strcat(news,childbuff);
								newsFilePath[0] = '\0';
								strcat(newsFilePath, acadFolderPath);
								strcat(newsFilePath, "/");
								childbuff[strlen(childbuff)-1] = '\0';
								strcat(newsFilePath, childbuff);
								strcat(newsFilePath, ".txt");
								FILE* newsFile = fopen(newsFilePath,"r");
								if(newsFile == NULL){
									perror("No such file on server.");
									exit(0);
								}
								if(flock(fileno(newsFile), LOCK_EX) == -1){
									perror("Error in accessing files");
									exit(0);
								}
								char childbufftemp[1023];
								newsFiles[fileCount][0] = '\0';
								while(fgets(childbufftemp, sizeof(childbufftemp), newsFile) != NULL){
									strcat(newsFiles[fileCount],childbufftemp);
								}
								fclose(newsFile);
								fileCount++;
							}
							fclose(acadFile);

							sprintf(childbuff,"%d",(int)strlen(news)+1);
							printf("size = %d\n", (int)strlen(news)+1);

							if(send(new_fd, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(writen(new_fd, news, strlen(news)+1) == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(strlen(news) <= 0)	continue;

							recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);

							if(recErr == -1){
								perror("Connection closed from Reader");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from Reader");
								exit(0);
							}

							int serial_No = atoi(childbuff);

							if(serial_No > fileCount){
								printf("Wrong input from client! Connection closed!");
								exit(0);
							}

							sprintf(childbuff,"%d",(int)strlen(newsFiles[serial_No-1])+1);
							printf("size = %d\n", (int)strlen(newsFiles[serial_No-1])+1);

							if(send(new_fd, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(writen(new_fd, newsFiles[serial_No-1], strlen(newsFiles[serial_No-1])+1) == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

						}
						else if(childbuff[0] == '2'){
							printf("Inside Non-academic\n");
							
							FILE* nonacadFile = fopen(nonacadFilePath,"a+");
							if(nonacadFile == NULL){
								perror("Error in opeing files");
								exit(0);
							}
							if(flock(fileno(nonacadFile), LOCK_EX) == -1){
								perror("Error in accessing files");
								exit(0);
							}
							int fileCount = 0;
							news[0] = '\0';
							//sleep(15);
							while(fgets(childbuff, sizeof(childbuff), nonacadFile) != NULL){
								strcat(news,childbuff);
								newsFilePath[0] = '\0';
								strcat(newsFilePath, nonacadFolderPath);
								strcat(newsFilePath, "/");
								childbuff[strlen(childbuff)-1] = '\0';
								strcat(newsFilePath, childbuff);
								strcat(newsFilePath, ".txt");
								FILE* newsFile = fopen(newsFilePath,"r");
								if(newsFile == NULL){
									perror("No such file on server.");
									exit(0);
								}
								if(flock(fileno(newsFile), LOCK_EX) == -1){
									perror("Error in accessing files");
									exit(0);
								}
								char childbufftemp[1023];
								newsFiles[fileCount][0] = '\0';
								while(fgets(childbufftemp, sizeof(childbufftemp), newsFile) != NULL){
									strcat(newsFiles[fileCount],childbufftemp);
								}
								fclose(newsFile);
								fileCount++;
							}
							fclose(nonacadFile);

							sprintf(childbuff,"%d",(int)strlen(news)+1);
							printf("size = %d\n", (int)strlen(news)+1);

							if(send(new_fd, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(writen(new_fd, news, strlen(news)+1) == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(strlen(news) <= 0)	continue;


							recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);

							if(recErr == -1){
								perror("Connection closed from Reader");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from Reader");
								exit(0);
							}

							int serial_No = atoi(childbuff);


							if(serial_No > fileCount){
								printf("Wrong input from client! Connection closed!");
								exit(0);
							}

							sprintf(childbuff,"%d",(int)strlen(newsFiles[serial_No-1])+1);
							printf("size = %d\n", (int)strlen(newsFiles[serial_No-1])+1);

							if(send(new_fd, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								exit(0);
							}

							if(writen(new_fd, newsFiles[serial_No-1], strlen(newsFiles[serial_No-1])+1) == -1){
								perror("Connection closed from Reader");
								exit(0);
							}
						}
					}
					printf("Connection closed with Reader\n");
				}
				else if(childbuff[0] == '2'){	//Reporter is connected via socket


					printf("Connection established with some Reporter\n");

					while(1){
						char healine[1023];
						recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);

						if(recErr == -1){
							perror("Connection closed from reporter");
							exit(0);
						}
						else if(recErr == 0){
							perror("Connection closed from reporter");
							exit(0);
						}	

						if(childbuff[0] == '1'){	
							printf("Inside academic\n");
							recErr = recv(new_fd, healine, BUFF_SIZE, 0);

							if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}	
							printf("\nDate and Headline %s\n",healine );

							

				            
				            newsFilePath[0] = '\0';
				            strcat(newsFilePath,acadFolderPath);
				            strcat(newsFilePath,"/");
				            strcat(newsFilePath,healine);
				            strcat(newsFilePath,".txt");
				            FILE* newsFile = fopen(newsFilePath,"a+"); 
				            if(newsFile == NULL){
								perror("Error in opeing files");
								exit(0);
							}  


				            recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);
				            if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}

				            int sizeofFile = atoi(childbuff);
				            printf("size of file = %d\n",sizeofFile );

				            recErr = readn(new_fd, news, sizeofFile);
				            if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}
							 
				            fprintf(newsFile,"%s",news);
				            fclose(newsFile);

				            writeHeading(healine, acadFilePath);

						}
						else if(childbuff[0] == '2'){
							printf("Inside Non-academic\n");
							recErr = recv(new_fd, healine, BUFF_SIZE, 0);

							if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}	
							printf("\nDate and Headline %s\n",healine );

							
				 


				            newsFilePath[0] = '\0';
				            strcat(newsFilePath,nonacadFolderPath);
				            strcat(newsFilePath,"/");
				            strcat(newsFilePath,healine);
				            strcat(newsFilePath,".txt");
				            printf("newsFilePath = %s\n",newsFilePath );
				            FILE* newsFile = fopen(newsFilePath,"a+"); 
				            if(newsFile == NULL){
								perror("Error in opeing files");
								exit(0);
							}  


				            recErr = recv(new_fd, childbuff, BUFF_SIZE, 0);
				            if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}

				            int sizeofFile = atoi(childbuff);
				            printf("size of file = %d\n",sizeofFile );

				            recErr = readn(new_fd, news, sizeofFile);
				            if(recErr == -1){
								perror("Connection closed from reporter");
								exit(0);
							}
							else if(recErr == 0){
								perror("Connection closed from reporter");
								exit(0);
							}
							 
				            fprintf(newsFile,"%s",news);
				            fclose(newsFile);
				            writeHeading(healine, nonacadFilePath);
						}
					}
					printf("Connection closed with Reporter\n");
				}
				close(new_fd);
				exit(0);
			}
			close(new_fd);
			
		}
	}
	else{	//For UDP coonection (administer)
		int UDPsockfd, sin_size;
		struct sockaddr_in server_addr, client_addr;
		UDPsockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(UDPsockfd == -1){
			perror("Error in creating Socket!");
			exit(0);
		}
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(UDP_SERVER_PORT);
		if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0){
			perror("Error in creating server.");
		}
		//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		memset(&(server_addr.sin_zero), '\0', 8);

		if(bind(UDPsockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
			perror("Error in Binding");
			exit(0);
		}
		while(1){
			printf("Waiting for Admin!\n");
			char buff[1023];
			sin_size = sizeof(struct sockaddr_in);
			int recErr =  recvfrom(UDPsockfd, buff, BUFF_SIZE, 0, (struct sockaddr *)&client_addr, &sin_size);
			printf("Connected to admin > lolxD\n");
			if(recErr == -1){
				perror("Connection lost1");
				exit(0);
			}
			else if(recErr == 0){
				perror("Connection lost2");
				exit(0);
			}
			printf("Received Password = %s\n",buff);

			if(strcmp(buff,"group")){
				buff[0] = '0';
				buff[1] = '\0';
				if(sendto(UDPsockfd, buff, BUFF_SIZE, 0,  (struct sockaddr *)&client_addr, sin_size) == -1){
					perror("Connection Lost:");
					continue;
				}
				continue;
			}
			buff[0] = '1';
			buff[1] = '\0';
			if(sendto(UDPsockfd, buff, BUFF_SIZE, 0,  (struct sockaddr *)&client_addr, sin_size) == -1){
				perror("Connection Lost:");
			}

			recErr =  recvfrom(UDPsockfd, buff, BUFF_SIZE, 0, (struct sockaddr *)&client_addr, &sin_size);
			if(recErr == -1){
				perror("Connection lost1");
				exit(0);
			}
			else if(recErr == 0){
				perror("Connection lost2");
				exit(0);
			}

			printf("Received Date from Admin = %s\n",buff);

			deletFiles(buff);
			printf("\nlol3\n");
			printf("Connection closed for Admin!\n");
		}

	}

	return 0;
}





void deletFiles(char* Date){

	char headings[1000][500];

	FILE* acadFile = fopen(acadFilePath,"a+");
	if(acadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(acadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}
	int i = 0, k = 0;
    while(fgets(headings[i++],500,acadFile) != NULL){
    	if(dateCompare(Date,headings[i-1])){
    		k++;
    	}
    }
    fclose(acadFile);

    i--;

    acadFile = fopen(acadFilePath,"w");
	if(acadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(acadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}
	int j=k;
	while(j<i){
		fprintf(acadFile, "%s",headings[j++] );
	}

	fclose(acadFile);

    j=0;
    while(j<k){
		char newsFilePath[300];
		newsFilePath[0] = '\0';
        strcat(newsFilePath,acadFolderPath);
        strcat(newsFilePath,"/");
        headings[j][strlen(headings[j]) -1 ] = '\0';
        strcat(newsFilePath,headings[j]);
        strcat(newsFilePath,".txt");
		if(remove(newsFilePath) == -1){
			perror("Error in Deleting files.");
			break;
		}
		j++;
	}

	
	FILE* nonacadFile = fopen(nonacadFilePath,"a+");
	if(nonacadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(nonacadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}
	i = 0, k = 0;
    while(fgets(headings[i++],500,nonacadFile) != NULL){
    	if(dateCompare(Date,headings[i-1])){
    		k++;
    	}
    }
    fclose(nonacadFile);

    i--;

    nonacadFile = fopen(nonacadFilePath,"w");
	if(nonacadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(nonacadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}
	j=k;
	while(j<i){
		fprintf(nonacadFile, "%s",headings[j++] );
	}

	fclose(nonacadFile);

    j=0;
    while(j<k){
		char newsFilePath[300];
		newsFilePath[0] = '\0';
        strcat(newsFilePath,nonacadFolderPath);
        strcat(newsFilePath,"/");
        headings[j][strlen(headings[j]) -1 ] = '\0';
        strcat(newsFilePath,headings[j]);
        strcat(newsFilePath,".txt");
		if(remove(newsFilePath) == -1){
			perror("Error in Deleting files.");
			break;
		}
		j++;
	}
	return;
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


ssize_t readn(int fd, void *buffer, size_t n)
{
	 ssize_t numRead; /* # of bytes fetched by last read() */
	 size_t totRead; /* Total # of bytes read so far */
	 char *buf;
	 buf = buffer; /* No pointer arithmetic on "void *" */

	 for (totRead = 0; totRead < n; ) {

		 numRead = recv(fd, buf, n - totRead, 0);

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


void writeHeading(char* newHeading, char* filePath){

	char headings[1000][500];
	bool headingWritten = false;

	FILE* acadFile = fopen(filePath,"a+");
	if(acadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(acadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}
	int i = 0;
    while(fgets(headings[i++],500,acadFile) != NULL);
    fclose(acadFile);

    i--;
    acadFile = fopen(filePath,"w");
	if(acadFile == NULL){
		perror("Error in opeing files");
		exit(0);
	}
	if(flock(fileno(acadFile), LOCK_EX) == -1){
		perror("Error in accessing files");
		exit(0);
	}

	int j = 0;
	printf("i = %d\n",i );
	while(j<i){
		if(dateCompare(newHeading,headings[j])){
			fprintf(acadFile, "%s",headings[j++] );
		}
		else 	break;
	}

	printf("newHeading  = %s  filename = %s \n",newHeading, filePath );
	fprintf(acadFile, "%s\n",newHeading );

	while(j<i){
		fprintf(acadFile, "%s",headings[j++] );
	}

	fclose(acadFile);
}


//Check if date 1 is greater than or equal to  date2
bool dateCompare(char*date1 , char*date2){

	char temp1[10], temp2[10];

	strncpy(temp1, &date1[6], 4);
	strncpy(temp2, &date2[6], 4);
	temp1[4] = temp2[4] = '\0';
	
	//printf("temp1 = %s temp2 = %s", temp1, temp2 );
	if(atoi(temp1) > atoi(temp2))		return true;
	else if (atoi(temp1) < atoi(temp2))	return false;

	strncpy(temp1, &date1[3], 2);
	strncpy(temp2, &date2[3], 2);
	temp1[2] = temp2[2] = '\0';
	
	//printf("temp1 = %s temp2 = %s", temp1, temp2 );
	if(atoi(temp1) > atoi(temp2))		return true;
	else if(atoi(temp1) < atoi(temp2))	return false;

	strncpy(temp1, &date1[0], 2);
	strncpy(temp2, &date2[0], 2);
	temp1[2] = temp2[2] = '\0';
	
	//printf("temp1 = %s temp2 = %s", temp1, temp2 );
	if(atoi(temp1) >= atoi(temp2))	return true;

	return false;

}
