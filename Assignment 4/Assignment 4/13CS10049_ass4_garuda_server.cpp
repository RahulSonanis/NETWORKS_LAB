/*
      Vishwas Jain      13CS10053
      Rahul Sonanis     13CS10049
      Assignment        4
      SMTP/POP  Server
*/

#include <time.h>
#include <vector>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define BUFFER_SIZE     1024
#define BACKLOG         20



int create_new_TCPSocket();
void bind_to_port(int, int, char*);
void start_listening(int);
void exiting(int);
void send_as_server(int, char*);
void receive_as_server(int, char*);
void exit_server(int);
void Initialising_connection(int, char*);
void verify_uname(char*, char*, int);
void verify_mailFrom(char*, char*, int);
void verify_rcpt(char*, char*, int);
void receive_mail_data(char*, int);
void listDirContentsAll(char*, int*, int*);
void verify_pass_pop(char*, char* , int);
void verify_uname_pop(char*, char*, int );
void retrieve_email(char*, int);
void list_all_emails(int);
bool verify_uname_server(char*, char*);
bool isInteger(char *);
void receive_mail_data_buffer(char*, int, char*, int);
void connect_server(int , char* ,int);
void bounce_back(char*);
ssize_t readn(int , void *, size_t );

/*
Error codes being implemented -
421 <domain> Service not available, closing transmission channel
550 Requested action not taken: mailbox unavailable
501 Syntax error in parameters or arguments
250 Requested mail action okay, completed
354 Start mail input; end with <CRLF>.<CRLF>
+OK
-ERR
*/

using namespace std;

typedef struct email{
      char filename[BUFFER_SIZE];
      int  msg_size;
      bool read;
}email;

vector<email> *emails;

int SMTP_sockfd, POP_sockfd;
socklen_t sin_size;
char  reverse_path[BUFFER_SIZE], forward_path[BUFFER_SIZE], pop_user[BUFFER_SIZE];


int main(int argc, char* argv[], char* env[]){


      signal(SIGINT, exit_server);

      if(argc != 7){
            printf("Usage ./garuda_server Your-IP-Address domain-name SMTP-PORT POP-PORT Relay-IP-Address Relay-SMTP-PORT\n");
            exit(0);
      }

      if(strcmp(argv[2], "abc.com") && strcmp(argv[2], "xyz.com")){
            printf("Usage ./garuda_server Your-IP-Address domain-name SMTP-PORT POP-PORT Relay-IP-Address Relay-SMTP-PORT\n");
            exit(0);
      }
      printf("%s Domain Server Started\n",argv[2]);

      sin_size = sizeof(struct sockaddr_in);


      SMTP_sockfd = create_new_TCPSocket();
      POP_sockfd = create_new_TCPSocket();
      bind_to_port(SMTP_sockfd, atoi(argv[3]), argv[1]);
      bind_to_port(POP_sockfd,atoi(argv[4]), argv[1]);
      start_listening(SMTP_sockfd);
      start_listening(POP_sockfd);

      if(!fork()){//For the SMTP server

            close(POP_sockfd);
            struct sockaddr_in garuda_client_addr;
            int garuda_client_fd;
            while(1){
                  garuda_client_fd = accept(SMTP_sockfd, (struct sockaddr *)&garuda_client_addr, &sin_size);
                  //printf("\n New garuda SMTP client connected with client sockfd = %d \n",garuda_client_fd);
                  if(garuda_client_fd == -1){
                        perror("Server Accept Failed");
                        continue;
                  }

                  if(!fork()){    //new SMTP client starts a connection
                        close(SMTP_sockfd);

                        char reply[BUFFER_SIZE];
                        char received[BUFFER_SIZE];

                        //HELO domain part
                        Initialising_connection(garuda_client_fd, argv[2]);

                        while(1){
                              char command[BUFFER_SIZE];
                              receive_as_server(garuda_client_fd, received);
                              strcpy(command, strtok(received," "));

                              if(!strcmp(command,"VRFY")){
                                    strcpy(received, strtok(NULL," "));
                                    verify_uname(received, argv[2], garuda_client_fd);
                              }
                              else if(!strcmp(command,"MAIL")){
                                    strtok(NULL, " ");
                                    strcpy(received, strtok(NULL," "));
                                    verify_mailFrom(received, argv[2], garuda_client_fd);
                              }
                              else if(!strcmp(command,"FORWARD")){
                                    sprintf(reply,"250 OK");
                                    send_as_server(garuda_client_fd, reply);
                                    strtok(NULL, " ");
                                    strcpy(received, strtok(NULL," "));
                                    sprintf(reverse_path,"%s",received);
                              }
                              else if(!strcmp(command,"RCPT")){
                                    strtok(NULL, " ");
                                    strcpy(received, strtok(NULL," "));
                                    verify_rcpt(received, argv[2], garuda_client_fd);
                              }
                              else if(!strcmp(command,"QUIT")){
                                    sprintf(reply,"221 <%s> Service closing transmission channel",argv[2]);
                                    send_as_server(garuda_client_fd, reply);
                                    break;
                              }
                              else if(!strcmp(command,"RSET")){
                                    sprintf(reply,"250 OK");
                                    send_as_server(garuda_client_fd, reply);
                                    forward_path[0] = '\0';
                                    reverse_path[0] = '\0';
                              }
                              else if(!strcmp(command,"DATA")){
                                    char to[BUFFER_SIZE];
                                    strcpy(to,forward_path);

                                    char* forward_path_uname = strtok(to,"@");
                                    char* forward_path_domain = strtok(NULL,"@");
                                    //printf("fpd = %s d = %s\n",forward_path_domain, argv[2] );
                                    if(!strcmp(forward_path_domain, argv[2])){
                                          //printf("lol1\n");
                                          receive_mail_data(argv[2], garuda_client_fd);
                                    }

                                    else{
                                          //printf("lol2\n" );
                                          receive_mail_data_buffer(argv[2], garuda_client_fd, argv[5], atoi(argv[6]));
                                    }


                              }
                        }

                        exiting(garuda_client_fd);
                  }
            }
      }
      else{//For the POP server
            close(SMTP_sockfd);
            struct sockaddr_in garuda_client_addr;
            int garuda_client_fd;
            while(1){
                  garuda_client_fd = accept(POP_sockfd, (struct sockaddr *)&garuda_client_addr, &sin_size);
                  //printf("\n New garuda pop client connected with client sockfd = %d \n",garuda_client_fd);
                  if(garuda_client_fd == -1){
                        perror("Server Accept Failed");
                        continue;
                  }

                  if(!fork()){    //new SMTP client starts a connection
                        close(POP_sockfd);
                        char reply[BUFFER_SIZE];
                        char received[BUFFER_SIZE];
                        emails = new vector<email>();
                        //Initialising the connection
                        sprintf(reply,"+OK POP server ready");
                        send_as_server(garuda_client_fd, reply);

                        while(1){
                              char command[BUFFER_SIZE];
                              receive_as_server(garuda_client_fd, received);
                              strcpy(command, strtok(received," "));

                              if(!strcmp(command,"USER")){
                                    strcpy(received, strtok(NULL," "));
                                    verify_uname_pop(received, argv[2], garuda_client_fd);
                              }
                              else if(!strcmp(command,"QUIT")){
                                    for(vector<email>::iterator it = emails->begin(); it != emails->end(); ++it) {
                                          if(it->read){
                                                remove(it->filename);
                                          }
                                    }
                                    sprintf(reply,"+OK POP server signing off\n");
                                    send_as_server(garuda_client_fd, reply);
                                    break;
                              }
                              else if(!strcmp(command,"PASS")){
                                    strcpy(received, strtok(NULL," "));
                                    verify_pass_pop(received, argv[2], garuda_client_fd);
                              }
                              else if(!strcmp(command,"STAT")){
                                    int total_msgs, total_size;
                                    char folder_name[BUFFER_SIZE];
                                    sprintf(folder_name,"%s/%s",argv[2],pop_user);
                                    listDirContentsAll(folder_name, &total_msgs, &total_size);
                                    sprintf(reply,"+OK You have %d unread messages (%d Bytes) ",total_msgs, total_size);
                                    send_as_server(garuda_client_fd, reply);
                              }
                              else if(!strcmp(command,"LIST")){
                                    list_all_emails(garuda_client_fd);
                              }
                              else if(!strcmp(command,"RETR")){
                                    strcpy(received, strtok(NULL," "));
                                    retrieve_email(received, garuda_client_fd);
                              }
                        }

                        if(emails != NULL)
                        delete emails;

                        exiting(garuda_client_fd);
                  }
            }
      }

}


void retrieve_email(char* received, int garuda_client_fd){
      char reply[BUFFER_SIZE];

      if(!isInteger(received)){
            sprintf(reply,"-ERR no such message\n");
            send_as_server(garuda_client_fd, reply);
            return;
      }

      int msg_no = atoi(received);

      if(msg_no>0 && msg_no<= emails->size()){

            email email_to_retreive = emails->at(msg_no-1);
            sprintf(reply,"+OK %dBytes\n",email_to_retreive.msg_size);

            send_as_server(garuda_client_fd, reply);
            FILE* message = fopen(email_to_retreive.filename,"r");

            if(message == NULL){
                  sprintf(reply,"-ERR no such message\n");
                  send_as_server(garuda_client_fd, reply);
                  return;
            }

            char line[BUFFER_SIZE];
            while(fgets(line, sizeof(line), message) != NULL){
                  sprintf(reply,"%s",line);
                  send_as_server(garuda_client_fd, reply);
            }
            (emails->at(msg_no-1)).read = true;
            fclose(message);
      }
      else{
            sprintf(reply,"-ERR no such message\n");
            send_as_server(garuda_client_fd, reply);
      }
}


// Used to check whether a given character is a numerical digit
bool isdigitmy(char c){
      if(c <= '9' && c >= '0')
      return true;
      else return false;
}

// Used to check whether a given string is a integer
bool isInteger(char *str){
      if (!*str)
      return false;

      // Check for non-digit chars in the rest of the stirng.
      while (*str)
      {
            if (!isdigitmy(*str))
            return false;
            else
            ++str;
      }

      return true;
}


void list_all_emails(int garuda_client_fd){
      char reply[BUFFER_SIZE];
      sprintf(reply,"+OK\n");
      send_as_server(garuda_client_fd, reply);

      int i=1;

      for(vector<email>::iterator it = emails->begin(); it != emails->end(); ++it) {
            sprintf(reply,"%d %dBytes\n", i++, it->msg_size);
            send_as_server(garuda_client_fd, reply);
      }
      sprintf(reply,".\n");
      send_as_server(garuda_client_fd, reply);
}


void verify_pass_pop(char* pass, char* domain, int garuda_client_fd){

      char received[BUFFER_SIZE];
      char reply[BUFFER_SIZE];



      char filename[BUFFER_SIZE];
      sprintf(filename,"%s.txt",domain);

      FILE* username_file = fopen(filename,"r");
      if(username_file == NULL){
            sprintf(reply,"-ERR invalid password");
            send_as_server(garuda_client_fd, reply);
            return;
      }
      //printf("pass = %s\n", pass);
      while(fgets(received, sizeof(received), username_file) != NULL){
            strtok(received,",");
            strtok(NULL,",");
            char* pass_stored =strtok(NULL,",");
            pass_stored[strlen(pass_stored)-1] = '\0';
            //printf("stored_pass = %s\n", pass_stored);
            if(!strcmp(pass,pass_stored)){
                  sprintf(reply,"+OK maildrop locked and ready");
                  send_as_server(garuda_client_fd, reply);
                  fclose(username_file);
                  return;
            }
      }
      fclose(username_file);
      sprintf(reply,"-ERR invalid password");
      send_as_server(garuda_client_fd, reply);
}


void verify_uname_pop(char* uname, char* domain, int garuda_client_fd){
      char reply[BUFFER_SIZE];
      if(verify_uname_server(uname,domain)){
            sprintf(reply,"+OK %s is welcome here\n",uname);
      }
      else{
            sprintf(reply,"-ERR never heard of %s\n",uname);
      }
      send_as_server(garuda_client_fd, reply);
}

void receive_mail_data_buffer(char* domain, int garuda_client_fd, char* SMTP_SERVER_IP, int SMTP_SERVER_PORT){
      char received[BUFFER_SIZE];
      char reply[BUFFER_SIZE];
      char to[BUFFER_SIZE];
      strcpy(to,forward_path);

      char* forward_path_uname = strtok(to,"@");
      char* forward_path_domain = strtok(NULL,"@");

      time_t rawtime;
      struct tm * timeinfo;
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      char filepath[BUFFER_SIZE];

      sprintf(filepath,"%s_%s_%s.txt",reverse_path,forward_path,asctime(timeinfo));
      FILE* email_file =  fopen(filepath, "a+");
      if(email_file == NULL){
            sprintf(reply,"451 Requested action aborted: local error in processing");
            send_as_server(garuda_client_fd, reply);
            return;
      }
      sprintf(reply,"354 Start mail input; end with <CRLF>.<CRLF>");
      send_as_server(garuda_client_fd, reply);

      // fprintf(email_file,"Mail From: %s\n",reverse_path);
      // fprintf(email_file,"Mail Body: ");

      while(1){
            receive_as_server(garuda_client_fd, received);
            fprintf(email_file,"%s",received);
            //printf("length = %d\n",(int)strlen(received));
            if(!strcmp(received,".\n"))   break;
      }

      printf("Message read in buffer successfully\n");
      sprintf(reply,"250 OK Message accepted for delivery ");
      send_as_server(garuda_client_fd, reply);
      fclose(email_file);


      if(!fork()){
            bool msg_forwarded = true;
            close(garuda_client_fd);
            char test[BUFFER_SIZE];
            char domain_name[BUFFER_SIZE];

            if(!strcmp(domain,"abc.com")){
                  sprintf(domain_name,"xyz.com");
            }
            else{
                  sprintf(domain_name,"abc.com");
            }
            int garuda_server_client_sockfd = create_new_TCPSocket();
            connect_server(garuda_server_client_sockfd, SMTP_SERVER_IP ,SMTP_SERVER_PORT);

            receive_as_server(garuda_server_client_sockfd, received);
            // waiting for 220 service ready
            strncpy(test,received,3);
            test[3]='\0';
            if((strcmp(test,"220") < 0) || ((strcmp(test,"220") > 0)))
            {
                  bounce_back(filepath);
                  remove(filepath);
                  exiting(garuda_server_client_sockfd);
            }

            // sending HELO command to server
            sprintf(reply,"HELO %s",domain_name);
            send_as_server(garuda_server_client_sockfd, reply);

            // waiting for server response to HELO
            receive_as_server(garuda_server_client_sockfd, received);

            // 421 <domain name> service not available checking error
            strncpy(test,received,3);
            test[3]='\0';
            if(!strcmp(test,"421"))
            {
                  bounce_back(filepath);
                  remove(filepath);
                  exiting(garuda_server_client_sockfd);
            }


            // SMTP connection established
            printf("\nSMTP connection established with server\n");

            sprintf(reply,"FORWARD FROM: %s",reverse_path);
            send_as_server(garuda_server_client_sockfd, reply);

            // waiting for server response to MAIL FROM  (no error checking as already checked in VRFY)
            receive_as_server(garuda_server_client_sockfd, received);

            sprintf(reply,"RCPT TO: %s",forward_path);
            send_as_server(garuda_server_client_sockfd, reply);

            // waiting for server response to RCPT TO
            receive_as_server(garuda_server_client_sockfd, received);

            // error checking for RCPT TO
            strncpy(test,received,3);
            test[3]='\0';
            if(!strcmp(test,"501") || !strcmp(test,"550"))
            {
                  bounce_back(filepath);
                  remove(filepath);
                  exiting(garuda_server_client_sockfd);
            }

            sprintf(reply,"DATA");
            send_as_server(garuda_server_client_sockfd, reply);

            // waiting for server response to DATA
            receive_as_server(garuda_server_client_sockfd, received);

            // error checking for DATA
            strncpy(test,received,3);
            test[3]='\0';
            if(!strcmp(test,"451"))
            {
                  bounce_back(filepath);
                  remove(filepath);
                  exiting(garuda_server_client_sockfd);
            }

            FILE* email_file =  fopen(filepath, "r");

            char line[BUFFER_SIZE];
            while(fgets(line, sizeof(line), email_file) != NULL){
                  sprintf(reply,"%s",line);
                  send_as_server(garuda_server_client_sockfd, reply);
            }
            fclose(email_file);

            remove(filepath);

            receive_as_server(garuda_server_client_sockfd, received);


            sprintf(reply,"QUIT");
            send_as_server(garuda_server_client_sockfd, reply);

            // waiting for server response to QUIT
            receive_as_server(garuda_server_client_sockfd, received);

            exiting(garuda_server_client_sockfd);
      }
}




void bounce_back(char* filepath1){
      time_t rawtime;
      struct tm * timeinfo;
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      char filepath[BUFFER_SIZE];
      char to[BUFFER_SIZE];
      strcpy(to,reverse_path);
      char* path_uname = strtok(to,"@");
      char* path_domain = strtok(NULL,"@");
      sprintf(filepath,"%s/%s/%s.txt",path_domain,path_uname,asctime(timeinfo));
      FILE* email_file =  fopen(filepath, "a+");

      fprintf(email_file,"Mail From: SMTP Server\n");
      fprintf(email_file,"Mail Body: Mail System Problem\nSorry, your mail to %s lost.\nMail you sent: \n",forward_path);
      FILE* sent_message = fopen(filepath1,"r");

      char line[BUFFER_SIZE];
      while(fgets(line, sizeof(line), sent_message) != NULL){
            fprintf(email_file,"%s", line);
      }
      fclose(sent_message);
      fclose(email_file);
}



void connect_server(int TCPsoc,char *arg1,int port)
{
      struct sockaddr_in srv_addr;
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


void receive_mail_data(char* domain, int garuda_client_fd){
      char received[BUFFER_SIZE];
      char reply[BUFFER_SIZE];
      char to[BUFFER_SIZE];
      strcpy(to,forward_path);
      char* forward_path_uname = strtok(to,"@");
      char* forward_path_domain = strtok(NULL,"@");
      time_t rawtime;
      struct tm * timeinfo;
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      char filepath[BUFFER_SIZE];

      sprintf(filepath,"%s/%s/%s.txt",forward_path_domain,forward_path_uname,asctime(timeinfo));
      FILE* email_file =  fopen(filepath, "a+");
      if(email_file == NULL){
            sprintf(reply,"451 Requested action aborted: local error in processing");
            send_as_server(garuda_client_fd, reply);
            return;
      }
      sprintf(reply,"354 Start mail input; end with <CRLF>.<CRLF>");
      send_as_server(garuda_client_fd, reply);

      fprintf(email_file,"Mail From: %s\n",reverse_path);
      fprintf(email_file,"Mail Body: \n");

      while(1){
            receive_as_server(garuda_client_fd, received);
            fprintf(email_file,"%s",received);
            //printf("length = %d\n",(int)strlen(received));
            if(!strcmp(received,".\n"))   break;
      }

      sprintf(reply,"250 OK Message accepted for delivery ");
      send_as_server(garuda_client_fd, reply);
      fclose(email_file);

}


void verify_mailFrom(char* received1, char* domain, int garuda_client_fd){

      char reply[BUFFER_SIZE];
      char received[BUFFER_SIZE];


      char* user_name, *domain_name;
      user_name = strtok(received1, "@");
      ////printf("username = %s\n", user_name);
      domain_name = strtok(NULL, "@");
      ////printf("uname = %s domain = %s",user_name,domain_name);
      if(user_name==NULL || domain_name==NULL){
            sprintf(reply,"501 Syntax error in parameters or arguments");
            send_as_server(garuda_client_fd, reply);
      }
      else if(strcmp(domain_name,domain)){
            sprintf(reply,"550 No such user");
            send_as_server(garuda_client_fd, reply);
      }
      else{
            char filename[BUFFER_SIZE];
            sprintf(filename,"%s.txt",domain);
            ////printf("filename = %s\n",filename );
            FILE* username_file = fopen(filename,"r");
            if(username_file == NULL){
                  sprintf(reply,"550 Requested action not taken: mailbox unavailable");
                  send_as_server(garuda_client_fd, reply);
                  return;
            }
            //printf("username = %s\n", user_name);
            while(fgets(received, sizeof(received), username_file) != NULL){
                  strtok(received,",");
                  ////printf("username = %s\n", user_name);
                  if(!strcmp(strtok(NULL,","),user_name)){
                        sprintf(reply,"250 OK");
                        sprintf(reverse_path,"%s@%s",user_name,domain);
                        send_as_server(garuda_client_fd, reply);
                        fclose(username_file);
                        return;
                  }
            }
            fclose(username_file);
            sprintf(reply,"550 Requested action not taken: mailbox unavailable");
            send_as_server(garuda_client_fd, reply);

      }
}




bool verify_uname_server(char* uname, char* domain){


      char received[BUFFER_SIZE];
      char* user_name, *domain_name;
      user_name = strtok(uname, "@");
      ////printf("username = %s\n", user_name);
      domain_name = strtok(NULL, "@");
      ////printf("uname = %s domain = %s",user_name,domain_name);
      if(user_name==NULL || domain_name==NULL){
            return false;
      }
      else if(strcmp(domain_name,domain)){
            return false;
      }
      else{
            char filename[BUFFER_SIZE];
            sprintf(filename,"%s.txt",domain);
            ////printf("filename = %s\n",filename );
            FILE* username_file = fopen(filename,"r");
            if(username_file == NULL){
                  return false;
            }
            ////printf("username = %s\n", user_name);
            while(fgets(received, sizeof(received), username_file) != NULL){
                  strtok(received,",");
                  ////printf("username = %s\n", user_name);
                  if(!strcmp(strtok(NULL,","),user_name)){
                        sprintf(pop_user,"%s",user_name);
                        fclose(username_file);
                        return true;
                  }
            }
            fclose(username_file);
            return false;
      }
}






void verify_rcpt(char* received1, char* domain, int garuda_client_fd){

      char reply[BUFFER_SIZE];
      char received[BUFFER_SIZE];

      // strcpy(received, received1);


      char* user_name, *domain_name;
      user_name = strtok(received1, "@");
      domain_name = strtok(NULL, "@");
      ////printf("uname = %s domain = %s",user_name,domain_name);
      if(user_name==NULL || domain_name==NULL){
            sprintf(reply,"501 Syntax error in parameters or arguments");
            send_as_server(garuda_client_fd, reply);
      }
      else if(strcmp(domain_name,"abc.com") && strcmp(domain_name, "xyz.com")){
            sprintf(reply,"550 Requested action not taken: mailbox unavailable");
            send_as_server(garuda_client_fd, reply);
      }
      else if(!strcmp(domain_name, domain)){
            char filename[BUFFER_SIZE];
            sprintf(filename,"%s.txt",domain);
            ////printf("filename = %s\n",filename );
            FILE* username_file = fopen(filename,"r");
            if(username_file == NULL){
                  sprintf(reply,"550 Requested action not taken: mailbox unavailable");
                  send_as_server(garuda_client_fd, reply);
                  return;
            }
            while(fgets(received, sizeof(received), username_file) != NULL){
                  strtok(received,",");
                  ////printf("username = %s\n", user_name);
                  if(!strcmp(strtok(NULL,","),user_name)){
                        sprintf(reply,"250 OK");
                        sprintf(forward_path,"%s@%s",user_name,domain);
                        send_as_server(garuda_client_fd, reply);
                        fclose(username_file);
                        return;
                  }
            }
            fclose(username_file);
            sprintf(reply,"550 Requested action not taken: mailbox unavailable");
            send_as_server(garuda_client_fd, reply);

      }
      else{//Email is for some other domain name
            sprintf(reply,"250 Requested mail action okay, completed");
            sprintf(forward_path,"%s@%s",user_name,domain_name);
            send_as_server(garuda_client_fd, reply);
      }
}


void verify_uname(char* received1, char* domain, int garuda_client_fd){

      char reply[BUFFER_SIZE];
      char received[BUFFER_SIZE];

      char line[BUFFER_SIZE];
      char temp[BUFFER_SIZE];

      //printf("lol\n" );
      int usernameExists = 0;
      char filename[BUFFER_SIZE];
      sprintf(filename,"%s.txt",domain);
      FILE* username_file = fopen(filename,"r");
      if(username_file == NULL){
            sprintf(reply,"550 Requested action not taken: mailbox unavailable");
            send_as_server(garuda_client_fd, reply);
            return;
      }

      while(fgets(received, sizeof(received), username_file) != NULL){
            strcpy(temp, received);
            char* full_name = strtok(temp,",");
            //printf("full name = %s\n", full_name);
            char* name = strtok(full_name, " ");
            while(name != NULL){
                  //printf("name = %s\n", name);
                  if(!strcasecmp(name,received1)){
                        usernameExists++;
                        strcpy(line, received);
                        break;
                  }
                  name = strtok(NULL," ");
            }
      }

      fclose(username_file);

      if(usernameExists == 0){
            sprintf(reply,"550 String does not match anything, user might not be local.");
            send_as_server(garuda_client_fd, reply);
      }
      else if(usernameExists == 1){
            char* name = strtok(line,",");
            char* email = strtok(NULL,",");
            sprintf(reply,"250 %s <%s@%s>",name,email,domain);
            send_as_server(garuda_client_fd, reply);
      }
      else{
            sprintf(reply,"553 User ambiguous.");
            send_as_server(garuda_client_fd, reply);

      }
}


void Initialising_connection(int garuda_client_fd, char* domain){
      char reply[BUFFER_SIZE];
      char received[BUFFER_SIZE];

      //Initialising the connection
      sprintf(reply,"220 <%s> Service Ready",domain);
      send_as_server(garuda_client_fd, reply);
      receive_as_server(garuda_client_fd, received);
      strtok(received," ");
      strcpy(received, strtok(NULL," "));
      if(strcmp(received, domain)){
            sprintf(reply,"421 <%s> Service not available, closing transmission channel",domain);
            send_as_server(garuda_client_fd, reply);
            exiting(garuda_client_fd);
      }

      sprintf(reply,"250 OK");
      send_as_server(garuda_client_fd, reply);
      printf("\nSMTP connection established with client\n");
}

ssize_t writen(int fd, const void *buffer, size_t n)
{
      ssize_t numWritten; /* # of bytes written by last write() */
      size_t totWritten; /* Total # of bytes written so far */
      const char *buf;
      buf = (char *)buffer; /* No pointer arithmetic on "void *" */

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

void send_as_server(int client_fd, char* buff){
      // printf("send as server = %s\n",buff);
      if(writen(client_fd, (char *)buff, BUFFER_SIZE) == -1){
            perror("Connection closed from Client");
            exiting(client_fd);
      }
}


void receive_as_server(int client_fd, char* buff){

      if(readn(client_fd, buff,  BUFFER_SIZE) == -1){
            perror("Connection closed from Client");
            exiting(client_fd);
      }
}


int create_new_TCPSocket(){
      int sockfd =  socket(AF_INET, SOCK_STREAM, 0);
      if(sockfd == -1){
            perror("Error in creating Socket!");
            exit(0);
      }
      //printf("\nSuccessfully created a socket with sockfd = %d\n",sockfd);
      return sockfd;
}


void bind_to_port(int TCPsockfd, int TCP_SERVER_PORT, char* ipaddr){
      struct sockaddr_in server_addr;
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(TCP_SERVER_PORT);
      if(inet_pton(AF_INET, ipaddr, &server_addr.sin_addr) <= 0){
            perror("Error in creating server.");
      }
      //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      memset(&(server_addr.sin_zero), '\0', 8);

      if(bind(TCPsockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
            perror("Error in Binding");
            exit(0);
      }

      //printf("\nSuccessfully binded the socket with sockfd = %d with port = %d\n",TCPsockfd, TCP_SERVER_PORT);
}


void start_listening(int TCPsockfd){
      if(listen(TCPsockfd, BACKLOG) == -1){
            perror("listen");
            exit(0);
      }
      //printf("\nStarted listening to new connections on sockfd = %d\n", TCPsockfd);
}



void exiting(int sockfd){
      close(sockfd);
      //printf("Closing the scoket with sockfd = %d .\n",sockfd);
      exit(0);
}

void exit_server(int signum){
      close(SMTP_sockfd);
      close(POP_sockfd);
      //printf("\nQuiting the server.\n");
      exit(0);
}



int selecterer(const struct dirent *a){
      size_t len = strlen(a->d_name);

      if(len > 4 && strcmp(a->d_name + len - 4, ".txt") == 0)     return 1;
      else return 0;
}


void listDirContentsAll(char* path, int* no_msg, int* total_size){

      emails->clear();
      struct dirent** dirEntry;
      char* filename;
      //The stat: It's how we'll retrieve the stats associated to the file.
      struct stat thestat;
      //will be used to determine the file owner & group
      struct passwd *pd;
      struct group *gp;

      int count = scandir(path, &dirEntry, &selecterer, NULL);
      //printf("path = %s\n", path);
      if(count == -1){
            perror("error");
            return;
      }

      char buff[BUFFER_SIZE];
      int al = 0;

      *no_msg = count;
      *total_size = 0;
      while(al != count){
            email new_email;
            filename = ((dirEntry)[al++])->d_name;
            sprintf(buff, "%s/%s", path, filename);
            strcpy(new_email.filename, buff);
            stat(buff, &thestat);
            new_email.msg_size = thestat.st_size;
            new_email.read = false;
            *total_size += new_email.msg_size;
            emails->push_back(new_email);
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
