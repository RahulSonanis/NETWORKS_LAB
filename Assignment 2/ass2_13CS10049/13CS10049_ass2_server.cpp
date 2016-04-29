/*
	Computer Networks Laboratory
	Assignment - 2 Ticket Counter
	Server side code
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
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
#include <vector>
#include <algorithm>
#include <signal.h>
 #include <sys/wait.h>



#undef max
#define TCP_SERVER_PORT 34568
#define UDP_SERVER_PORT 23456
#define BUFF_SIZE 1024
#define BACKLOG 20
#define max(x,y) ((x) > (y) ? (x) : (y))

using namespace std;



typedef struct coach
{
	int noOfSeats;
	bool *seats;
}coach;

typedef struct train
{
	int bookedAcSeats;
	int availableAcSeats;
	int bookedSlSeats;
	int availableSlSeats;
	int trainNumber;
	int countACCoach;
	int countSLCoach;
	coach *AcCoach;
	coach *SlCoach;

}train;


typedef struct bookingRequest
{
	int clientFD;
	int passID;
	int trainNo;
	char* coachType;
	int seatCount;
	char* preference;
	int* age;
	int avgOldAge;
	int oldPass;
}bookingRequest;

struct less_than_key
{
    inline bool operator() (const bookingRequest& req1, const bookingRequest& req2)
    {
    	if(req1.seatCount < req2.seatCount)
    		return true;
    	if(req1.oldPass < req2.oldPass)
    		return true;
    	if(req1.avgOldAge < req2.avgOldAge)
    		return true;

    	return true;
    }
};

typedef struct bookedSeats
{
	int CoachNumber;
	int SeatNumber;
}bookedSeats;


int TCPsockfd;
train superFastExp;
train rajdhaniExp;
std::vector<bookingRequest> requests;


void displayCurrentStatus();
void collectRequests(char*, int);
void processRequests();
void exiting(int );
void allocateSeats(bookingRequest);
int findCoach(train* , char* , char* , int );

int main(){
	signal(SIGINT,exiting);

	superFastExp.trainNumber = 12321;
	superFastExp.countACCoach = 3;
	superFastExp.countSLCoach = 12;
	superFastExp.AcCoach = new coach[3];
	superFastExp.SlCoach = new coach[12];
	superFastExp.bookedSlSeats = 0;
	superFastExp.availableSlSeats = 12*72;
	superFastExp.availableAcSeats = 3*72;
	superFastExp.bookedAcSeats = 0;
	for (int i = 0; i < superFastExp.countACCoach; ++i)
	{	
		superFastExp.AcCoach[i].noOfSeats = 72;
		superFastExp.AcCoach[i].seats = new bool[72];
		fill_n(superFastExp.AcCoach[i].seats, superFastExp.AcCoach[i].noOfSeats, true);
	}
	for (int i = 0; i < superFastExp.countSLCoach; ++i)
	{
		superFastExp.SlCoach[i].seats = new bool[72];
		superFastExp.SlCoach[i].noOfSeats = 72;
		fill_n(superFastExp.SlCoach[i].seats, superFastExp.SlCoach[i].noOfSeats, true);
	}
	rajdhaniExp.trainNumber = 12301;
	rajdhaniExp.countACCoach = 13;
	rajdhaniExp.countSLCoach = 0;
	rajdhaniExp.AcCoach = new coach[13];
	rajdhaniExp.bookedSlSeats = 0;
	rajdhaniExp.availableSlSeats = 0;
	rajdhaniExp.availableAcSeats = 13*54;
	rajdhaniExp.bookedAcSeats = 0;
	for (int i = 0; i < rajdhaniExp.countACCoach; ++i)
	{
		rajdhaniExp.AcCoach[i].seats = new bool[54];
		rajdhaniExp.AcCoach[i].noOfSeats = 54;
		fill_n(rajdhaniExp.AcCoach[i].seats, rajdhaniExp.AcCoach[i].noOfSeats, true);
	}



	std::vector<int> connectedFD;
	int sin_size, new_fd;
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
	if(inet_pton(AF_INET, "10.109.28.65", &server_addr.sin_addr) <= 0){
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
	//printf("TCP\n");
	while(1){
		//printf("Starting of the loop\n");
		int nfds = 0, readyFDCount;
        fd_set rd, wr, er;
        FD_ZERO(&rd);
        FD_ZERO(&wr);
        FD_ZERO(&er);
        FD_SET(TCPsockfd, &rd);
        nfds = max(nfds, TCPsockfd);
        FD_SET(0, &rd);
        nfds = max(nfds, 0);
        for (std::vector<int>::iterator i = connectedFD.begin(); i != connectedFD.end(); ++i)
        {
        	FD_SET(*i, &rd);
        	nfds = max(nfds, *i);
        }
        struct timeval tv;
        sleep(5);
        //sleep(15);
        // tv.tv_sec = 2;
        // tv.tv_usec = 0;
        readyFDCount = select(nfds + 1, &rd, NULL, NULL, 0);
        //printf("after sleep\n");

        if (readyFDCount == -1 && errno == EINTR)
           continue;

       	if (readyFDCount == -1) {
           perror("select()");
           exit(EXIT_FAILURE);
       	}


       	if(FD_ISSET(0, &rd)){
       		char userInput[250];
       		fgets(userInput, sizeof(userInput), stdin);
       		
   			userInput[strlen(userInput)-1] = '\0';
       		if(!strcmp(userInput,"1")){
       			int childpid = fork();
       			if(childpid < 0){
       				perror("\nError in forking child, Exiting!\n");
       				exit(0);
       			}
       			else if(childpid == 0){
       				displayCurrentStatus();
       				exit(0);
       			}
       		}
       		else{
       			printf("\nWrong user Input! Enter '1' to see the current booking status of all the trains\n");
       		}
       		
       		
       	}


        if (FD_ISSET(TCPsockfd, &rd)) {
        	socklen_t sin_size = sizeof(struct sockaddr_in);
        	
    		new_fd = accept(TCPsockfd, (struct sockaddr *)&client_addr, &sin_size);
   
            if (new_fd == -1) {
                perror("accept()");
            } else {
           		connectedFD.push_back(new_fd);
     			printf("New Client connected!\n");
            }
       }
       //printf("\nBefore checking requests\n");
	    for (std::vector<int>::iterator i = connectedFD.begin(); i != connectedFD.end();)
	    {

	    	char childbuff[BUFF_SIZE];
	    	if (FD_ISSET(*i, &rd)){
	    		int recErr = recv(*i, childbuff, BUFF_SIZE, 0);
	    		//printf("receive\n");
				if(recErr == -1){
					perror("Connection lost1");
					exit(0);
				}
				else if(recErr == 0){
					printf("\nClient with sockfd = %d disconnected\n", *i);
					connectedFD.erase(i);
					// perror("Connection lost2");
					// exit(0);
				}
				else{
					collectRequests(childbuff, *i);
					i++;
					//printf("lol\n");
				}
				
	    	}
	    	else i++;
	    }
	    //printf("Calling processRequests\n");
	    if(!requests.empty()){
	    	processRequests();
	    	requests.clear();
	    }
	    
	}
}


void collectRequests(char* input, int clientFileD){

	bookingRequest req;
	req.oldPass = 0;
	req.clientFD = clientFileD;

	char* token = strtok(input,",");
	req.passID = atoi(token);
	//printf("lol\n");
	token = strtok(NULL,",");
	req.trainNo = atoi(token);
	//printf("lol\n");
	token = strtok(NULL,",");
	req.coachType = strdup(token);
	//printf("lol\n");
	token = strtok(NULL,",");
	req.seatCount = atoi(token);
	//printf("lol\n");

	token = strtok(NULL,",");
	req.preference = strdup(token);
	//printf("lol\n");
	int i = 0;
	int avg=0,n=0; // 
	req.age = new int[req.seatCount];
	while(i < req.seatCount){
		token = strtok(NULL,"-");
		req.age[i] = atoi(token);

		//
		if(req.age[i] >= 60)
		{
			avg += req.age[i];
			n++;
		}
		i++;
		//
	}
	if(n>0)
		avg /= n;
	req.oldPass = n;
	req.avgOldAge = avg;

	requests.push_back(req);
}



void processRequests()
{
	std::sort(requests.begin(), requests.end(), less_than_key());
	std::reverse(requests.begin(), requests.end());
	//printf("\n\nPrinting Requests after sorting from high to low preference\n");
	for (std::vector<bookingRequest>::iterator it = requests.begin(); it != requests.end();)
	{
		//printf("\n\n\nclientFD = %d\npassID = %d\ntrainNp = %d\nseatCount = %d\n",it->clientFD,it->passID,it->trainNo,it->seatCount);
		allocateSeats(*it);
		requests.erase(it);
	}
}

void displayCurrentStatus(){
	printf("\t----------------------------------------------------------------------------------------------------\n");
	printf("\n\t\tTrain\t\t\t\t#seats\t\t\t\t#seats\n\t\t\t\t\tTotal[Booked/Available]\t\tTotal[Booked/Available]\n");
	printf("\t\t\t\t\t\t(AC)\t\t\t\t(Sleeper)\n");
	printf("\t----------------------------------------------------------------------------------------------------\n");
	printf("\t\tSuperfast Exp\t\t\t%d[%d/%d]\t\t\t%d[%d/%d]\n",72*3,superFastExp.bookedAcSeats,superFastExp.availableAcSeats,72*12,superFastExp.bookedSlSeats,superFastExp.availableSlSeats);
	printf("\t\tRajdhani Exp \t\t\t%d[%d/%d]\t\t\t-\n",54*13,rajdhaniExp.bookedAcSeats,rajdhaniExp.availableAcSeats);
	printf("\t----------------------------------------------------------------------------------------------------\n");
}


void allocateSeats(bookingRequest req){
	//printf("\nIn Allocate seats funciton\n");
	char childbuff[BUFF_SIZE];
	train* trainToBook;
	if(req.trainNo == rajdhaniExp.trainNumber)
		trainToBook = &rajdhaniExp;
	else if(req.trainNo == superFastExp.trainNumber)
		trainToBook = &superFastExp;
	else{
		sprintf(childbuff,"NA");
		if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
			perror("Connection closed from Reader");
		}
		return;
	}

	if(!strcmp(req.coachType,"AC")){
		if(req.seatCount > trainToBook->availableAcSeats){
			sprintf(childbuff,"NA");
			if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
				perror("Connection closed from Reader");
			}
			return;
		}
		//printf("\nIn AC\n");
		int AvailCoachNumer = findCoach(trainToBook, req.preference, req.coachType, req.seatCount);
		if(strcmp(req.preference,"NA")){
			int sideUpper, sideLower, upper, lower, middle;
			sideUpper = sideLower = upper = lower = middle = 0;
			char* token;
			char* pref = strdup(req.preference);
			token = strtok(pref,"-");
			
			while(token != NULL){
				if(!strcmp(token,"SU"))			sideUpper++;
				else if(!strcmp(token,"SL"))	sideLower++;
				else if(!strcmp(token,"UB"))	upper++;
				else if(!strcmp(token,"LB"))	lower++;
				else if(!strcmp(token,"MB"))	middle++;
				token = strtok(NULL,"-");
			}
			int sideUpperCount, sideLowerCount, upperCount, lowerCount, middleCount;
			sideUpperCount = sideLowerCount = upperCount = lowerCount = middleCount = 0;
			std::vector<bookedSeats> seatsAllocated;
			if(AvailCoachNumer >= 0){
				for (int i = 0; i < trainToBook->AcCoach[AvailCoachNumer].noOfSeats ; ++i){
					if(trainToBook->AcCoach[AvailCoachNumer].seats[i]){
						if(trainToBook == &superFastExp){
							if((i+1)%8 == 0){
								if(sideUpperCount < sideUpper){
									sideUpperCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}					
							else if((i+2)%8 == 0){
								if(sideLowerCount < sideLower){
									sideLowerCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}					
							else if((i+6)%8 == 0 || (i+3)%8 == 0){
								if(upperCount < upper){
									upperCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}	
							else if((i-4)%8 == 0 || (i-1)%8 == 0){
								if(middleCount < middle){
									middleCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}	
							else{
								if(lowerCount < lower){
									lowerCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}									
						}
						else if(trainToBook == &rajdhaniExp){
							if((i+1)%6 == 0){
								if(sideUpperCount < sideUpper){
									sideUpperCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}
							else if((i+2)%6 == 0){
								if(sideLowerCount < sideLower){
									sideLowerCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}
							else if((i+5)%6 == 0 || (i+3)%6 == 0){
								if(upperCount < upper){
									upperCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}
							else{
								if(lowerCount < lower){
									lowerCount++;
									bookedSeats temp;
									temp.CoachNumber = AvailCoachNumer;
									temp.SeatNumber = i;
									seatsAllocated.push_back(temp);
								}
							}
						}
						if(seatsAllocated.size() == req.seatCount){
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"A%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->AcCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedAcSeats += req.seatCount;
							trainToBook->availableAcSeats -= req.seatCount;
							return;
						}
					}
				}
			}
			else{
				bool boolkingSuccess = false;
				for (int i = 0; i < trainToBook->countACCoach; ++i)
				{
					for (int j = 0; j < trainToBook->AcCoach[i].noOfSeats ; ++j)
					{
						if(trainToBook->AcCoach[i].seats[j]){
							if(trainToBook == &superFastExp){
								if((j+1)%8 == 0){
									if(sideUpperCount < sideUpper){
										sideUpperCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}					
								else if((j+2)%8 == 0){
									if(sideLowerCount < sideLower){
										sideLowerCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}					
								else if((j+6)%8 == 0 || (j+3)%8 == 0){
									if(upperCount < upper){
										upperCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}	
								else if((j-4)%8 == 0 || (j-1)%8 == 0){
									if(middleCount < middle){
										middleCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}	
								else{
									if(lowerCount < lower){
										lowerCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}									
							}
							else if(trainToBook == &rajdhaniExp){
								if((j+1)%6 == 0){
									if(sideUpperCount < sideUpper){
										sideUpperCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}
								else if((j+2)%6 == 0){
									if(sideLowerCount < sideLower){
										sideLowerCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}
								else if((j+5)%6 == 0 || (j+3)%6 == 0){
									if(upperCount < upper){
										upperCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}
								else{
									if(lowerCount < lower){
										lowerCount++;
										bookedSeats temp;
										temp.CoachNumber = i;
										temp.SeatNumber = j;
										seatsAllocated.push_back(temp);
									}
								}
							}
						}
						if(seatsAllocated.size() == req.seatCount){
							boolkingSuccess = true;
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"A%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->AcCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedAcSeats += req.seatCount;
							trainToBook->availableAcSeats -= req.seatCount;
							return;
						}
					}
				}
				if(!boolkingSuccess){
					sprintf(childbuff,"NA");
					if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
						perror("Connection closed from Reader");
					}
					return;
				}
			}
		}
		else{
			std::vector<bookedSeats> seatsAllocated;
			if(AvailCoachNumer >= 0){
				for (int i = 0; i < trainToBook->AcCoach[AvailCoachNumer].noOfSeats ; ++i){
					if(trainToBook->AcCoach[AvailCoachNumer].seats[i]){
						bookedSeats temp;
						temp.CoachNumber = AvailCoachNumer;
						temp.SeatNumber = i;
						seatsAllocated.push_back(temp);
					}
					if(seatsAllocated.size() == req.seatCount){
						childbuff[0] = '\0';
						char temp1[10];
						for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
						{
							sprintf(temp1,"A%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
							strcat(childbuff,temp1);
						}
						childbuff[strlen(childbuff) - 1] = '\0';
						if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
							perror("Connection closed from Reader");
							return;
							//exit(0);
						}
						for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
						{
							trainToBook->AcCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
							
						}
						trainToBook->bookedAcSeats += req.seatCount;
						trainToBook->availableAcSeats -= req.seatCount;
						return;
					}
				}
			}
			else{
				for (int i = 0; i < trainToBook->countACCoach; ++i)
				{
					for (int j = 0; j < trainToBook->AcCoach[i].noOfSeats ; ++j)
					{
						if(trainToBook->AcCoach[i].seats[j]){
							bookedSeats temp;
							temp.CoachNumber = i;
							temp.SeatNumber = j;
							seatsAllocated.push_back(temp);
						}
						if(seatsAllocated.size() == req.seatCount){
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"A%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->AcCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedAcSeats += req.seatCount;
							trainToBook->availableAcSeats -= req.seatCount;
							return;
						}
					}
				}
			}
		}
		
	}
	else if(!strcmp(req.coachType,"Sleeper")){
		if(req.seatCount > trainToBook->availableSlSeats){
			sprintf(childbuff,"NA");
			if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
				perror("Connection closed from Reader");
			}
			return;
		}
		//printf("\nIn Sleeper\n");
		int AvailCoachNumer = findCoach(trainToBook, req.preference, req.coachType, req.seatCount);
		if(strcmp(req.preference,"NA")){
			
			int sideUpper, sideLower, upper, lower, middle;
			sideUpper = sideLower = upper = lower = middle = 0;
			char* token;
			char* pref = strdup(req.preference);
			token = strtok(pref,"-");
			
			while(token != NULL){
				if(!strcmp(token,"SU"))			sideUpper++;
				else if(!strcmp(token,"SL"))	sideLower++;
				else if(!strcmp(token,"UB"))	upper++;
				else if(!strcmp(token,"LB"))	lower++;
				else if(!strcmp(token,"MB"))	middle++;
				token = strtok(NULL,"-");
			}
			int sideUpperCount, sideLowerCount, upperCount, lowerCount, middleCount;
			sideUpperCount = sideLowerCount = upperCount = lowerCount = middleCount = 0;
			std::vector<bookedSeats> seatsAllocated;
			if(AvailCoachNumer >= 0){
				for (int i = 0; i < trainToBook->SlCoach[AvailCoachNumer].noOfSeats ; ++i){
					if(trainToBook->SlCoach[AvailCoachNumer].seats[i]){
						if((i+1)%8 == 0){
							if(sideUpperCount < sideUpper){
								sideUpperCount++;
								bookedSeats temp;
								temp.CoachNumber = AvailCoachNumer;
								temp.SeatNumber = i;
								seatsAllocated.push_back(temp);
							}
						}					
						else if((i+2)%8 == 0){
							if(sideLowerCount < sideLower){
								sideLowerCount++;
								bookedSeats temp;
								temp.CoachNumber = AvailCoachNumer;
								temp.SeatNumber = i;
								seatsAllocated.push_back(temp);
							}
						}					
						else if((i+6)%8 == 0 || (i+3)%8 == 0){
							if(upperCount < upper){
								upperCount++;
								bookedSeats temp;
								temp.CoachNumber = AvailCoachNumer;
								temp.SeatNumber = i;
								seatsAllocated.push_back(temp);
							}
						}	
						else if((i-4)%8 == 0 || (i-1)%8 == 0){
							if(middleCount < middle){
								middleCount++;
								bookedSeats temp;
								temp.CoachNumber = AvailCoachNumer;
								temp.SeatNumber = i;
								seatsAllocated.push_back(temp);
							}
						}	
						else{
							if(lowerCount < lower){
								lowerCount++;
								bookedSeats temp;
								temp.CoachNumber = AvailCoachNumer;
								temp.SeatNumber = i;
								seatsAllocated.push_back(temp);
							}
						}
						if(seatsAllocated.size() == req.seatCount){
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"S%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->SlCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedSlSeats += req.seatCount;
							trainToBook->availableSlSeats -= req.seatCount;
							return;
						}
					}
				}
			}
			else{
				bool boolkingSuccess = false;
				for (int i = 0; i < trainToBook->countSLCoach; ++i)
				{
					for (int j = 0; j < trainToBook->SlCoach[i].noOfSeats ; ++j)
					{
						if(trainToBook->SlCoach[i].seats[j]){
							if((j+1)%8 == 0){
								if(sideUpperCount < sideUpper){
									sideUpperCount++;
									bookedSeats temp;
									temp.CoachNumber = i;
									temp.SeatNumber = j;
									seatsAllocated.push_back(temp);
								}
							}					
							else if((j+2)%8 == 0){
								if(sideLowerCount < sideLower){
									sideLowerCount++;
									bookedSeats temp;
									temp.CoachNumber = i;
									temp.SeatNumber = j;
									seatsAllocated.push_back(temp);
								}
							}					
							else if((j+6)%8 == 0 || (j+3)%8 == 0){
								if(upperCount < upper){
									upperCount++;
									bookedSeats temp;
									temp.CoachNumber = i;
									temp.SeatNumber = j;
									seatsAllocated.push_back(temp);
								}
							}	
							else if((j-4)%8 == 0 || (j-1)%8 == 0){
								if(middleCount < middle){
									middleCount++;
									bookedSeats temp;
									temp.CoachNumber = i;
									temp.SeatNumber = j;
									seatsAllocated.push_back(temp);
								}
							}	
							else{
								if(lowerCount < lower){
									lowerCount++;
									bookedSeats temp;
									temp.CoachNumber = i;
									temp.SeatNumber = j;
									seatsAllocated.push_back(temp);
								}
							}
						}
						if(seatsAllocated.size() == req.seatCount){
							boolkingSuccess = true;
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"S%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->SlCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedSlSeats += req.seatCount;
							trainToBook->availableSlSeats -= req.seatCount;
							return;
						}
					}
				}
				if(!boolkingSuccess){
					sprintf(childbuff,"NA");
					if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
						perror("Connection closed from Reader");
					}
					return;
				}
			}
		}
		else{
			std::vector<bookedSeats> seatsAllocated;
			if(AvailCoachNumer >= 0){
				for (int i = 0; i < trainToBook->SlCoach[AvailCoachNumer].noOfSeats ; ++i){
					if(trainToBook->SlCoach[AvailCoachNumer].seats[i]){
						bookedSeats temp;
						temp.CoachNumber = AvailCoachNumer;
						temp.SeatNumber = i;
						seatsAllocated.push_back(temp);
					}
					if(seatsAllocated.size() == req.seatCount){
						childbuff[0] = '\0';
						char temp1[10];
						for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
						{
							sprintf(temp1,"S%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
							strcat(childbuff,temp1);
						}
						childbuff[strlen(childbuff) - 1] = '\0';
						if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
							perror("Connection closed from Reader");
							return;
							//exit(0);
						}
						for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
						{
							trainToBook->SlCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
							
						}
						trainToBook->bookedSlSeats += req.seatCount;
						trainToBook->availableSlSeats -= req.seatCount;
						return;
					}
				}
			}
			else{
				for (int i = 0; i < trainToBook->countSLCoach; ++i)
				{
					for (int j = 0; j < trainToBook->SlCoach[i].noOfSeats ; ++j)
					{
						if(trainToBook->SlCoach[i].seats[j]){
							bookedSeats temp;
							temp.CoachNumber = i;
							temp.SeatNumber = j;
							seatsAllocated.push_back(temp);
						}
						if(seatsAllocated.size() == req.seatCount){
							childbuff[0] = '\0';
							char temp1[10];
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								sprintf(temp1,"S%d(%d)-",ite->CoachNumber+1,ite->SeatNumber+1);
								strcat(childbuff,temp1);
							}
							childbuff[strlen(childbuff) - 1] = '\0';
							if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
								perror("Connection closed from Reader");
								return;
								//exit(0);
							}
							for (std::vector<bookedSeats>::iterator ite = seatsAllocated.begin(); ite != seatsAllocated.end(); ++ite)
							{
								trainToBook->SlCoach[ite->CoachNumber].seats[ite->SeatNumber] = false;
								
							}
							trainToBook->bookedSlSeats += req.seatCount;
							trainToBook->availableSlSeats -= req.seatCount;
							return;
						}
					}
				}
			}
		}
	}
	else{
		sprintf(childbuff,"NA");
		if(send(req.clientFD, childbuff, BUFF_SIZE, 0)  == -1){
			perror("Connection closed from Reader");
		}
		return;
	}
}


//Finding the same coach for all the passengers in the same request.
int findCoach(train* trainToBook, char* preference, char* coachType, int seatCount){
	if(!strcmp(coachType,"AC")){
		if(!strcmp(preference,"NA")){
			for (int i = 0; i < trainToBook->countACCoach; ++i)
			{
				int k = 0;
				for (int j = 0; j < trainToBook->AcCoach[i].noOfSeats ; ++j)
				{
					if(trainToBook->AcCoach[i].seats[j]){
						k++;
						if(k >= seatCount)	return	i;
					}	
				}
			}
		}
		else{
			int sideUpper, sideLower, upper, lower, middle;
			sideUpper = sideLower = upper = lower = middle = 0;
			char* token;
			char* pref = strdup(preference);
			token = strtok(pref,"-");
			
			while(token != NULL){
				if(!strcmp(token,"SU"))			sideUpper++;
				else if(!strcmp(token,"SL"))	sideLower++;
				else if(!strcmp(token,"UB"))	upper++;
				else if(!strcmp(token,"LB"))	lower++;
				else if(!strcmp(token,"MB"))	middle++;
				token = strtok(NULL,"-");
			}
			for (int i = 0; i < trainToBook->countACCoach; ++i)
			{
				int sideUpperCount, sideLowerCount, upperCount, lowerCount, middleCount;
				sideUpperCount = sideLowerCount = upperCount = lowerCount = middleCount = 0;
				for (int j = 0; j < trainToBook->AcCoach[i].noOfSeats ; ++j)
				{
					if(trainToBook->AcCoach[i].seats[j]){
						if(trainToBook == &superFastExp){
							if((j+1)%8 == 0)						sideUpperCount++;
							else if((j+2)%8 == 0)					sideLowerCount++;
							else if((j+6)%8 == 0 || (j+3)%8 == 0)	upperCount++;
							else if((j-4)%8 == 0 || (j-1)%8 == 0)	middleCount++;
							else									lowerCount++;
						}
						else if(trainToBook == &rajdhaniExp){
							if((j+1)%6 == 0)						sideUpperCount++;
							else if((j+2)%6 == 0)					sideLowerCount++;
							else if((j+5)%6 == 0 || (j+3)%6 == 0)	upperCount++;
							else									lowerCount++;
						}
						if(sideUpper==sideUpperCount && sideLower==sideLowerCount && middle==middleCount && lower==lowerCount && upper==upperCount){
							return	i;
						}
					}

				}
			}
		}
	}
	else
	{
		if(!strcmp(preference,"NA")){
			for (int i = 0; i < trainToBook->countSLCoach; ++i)
			{
				int k = 0;
				for (int j = 0; j < trainToBook->SlCoach[i].noOfSeats ; ++j)
				{
					if(trainToBook->SlCoach[i].seats[j])	k++;
					if(k == seatCount)	return	i;
				}
			}
		}
		else{
			int sideUpper, sideLower, upper, lower, middle;
			sideUpper = sideLower = upper = lower = middle = 0;
			char* token;
			char* pref = strdup(preference);
			token = strtok(pref,"-");
			
			while(token != NULL){
				if(!strcmp(token,"SU"))			sideUpper++;
				else if(!strcmp(token,"SL"))	sideLower++;
				else if(!strcmp(token,"UB"))	upper++;
				else if(!strcmp(token,"LB"))	lower++;
				else if(!strcmp(token,"MB"))	middle++;
				token = strtok(NULL,"-");
			}
			for (int i = 0; i < trainToBook->countSLCoach; ++i)
			{
				int sideUpperCount, sideLowerCount, upperCount, lowerCount, middleCount;
				sideUpperCount = sideLowerCount = upperCount = lowerCount = middleCount = 0;
				for (int j = 0; j < trainToBook->SlCoach[i].noOfSeats ; ++j)
				{
					if(trainToBook->SlCoach[i].seats[j]){
						if((j+1)%8 == 0)						sideUpperCount++;
						else if((j+2)%8 == 0)					sideLowerCount++;
						else if((j+6)%8 == 0 || (j+3)%8 == 0)	upperCount++;
						else if((j-4)%8 == 0 || (j-1)%8 == 0)	middleCount++;
						else									lowerCount++;

						if(sideUpper==sideUpperCount && sideLower==sideLowerCount && middle==middleCount && lower==lowerCount && upper==upperCount){
							return	i;
						}
					}

				}
			}
		}
	}

	return -1;
}

void exiting(int signum){
	if(close(TCPsockfd) == -1){
		perror("Server encountered an Error while executing!\nExiting!");
		exit(0);
	}
	printf("Closing the server.\n");
	exit(0);
}

