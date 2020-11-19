#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
// #define PORT 8080 
#define MAXDATASIZE 128
int main(int argc, char const *argv[]) 
{ 
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	char *hello; 
	time_t rawtime; 
	struct tm * timeinfo; 
	int pid;
	int PORT = 0;
	if (argc != 2) {
	    fprintf(stderr,"usage: echos Port\n");
	    exit(1);
	}
	else 
		PORT = atoi(argv [1]);
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		perror("bind"); 
		exit(EXIT_FAILURE); 
	}
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	}
	printf("server: waiting for connections.....\n");
	while(1){
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
							(socklen_t*)&addrlen))<0) 
				{ 
					perror("accept"); 
					exit(1); 
				} 
		pid = fork (); 
		if(pid < 0){
			perror("fork");
			exit(1); 
		}
		if (pid == 0){
			// This is the client process
			printf("\nserver: new client connected \n");
			close(server_fd);
			while(1){
				valread = read(new_socket , buffer, 1024);
				time (&rawtime); 
				if(valread == 0){
					printf("\nCLIENT DONE, Closing this connection...\n");
					// close(new_socket);
					exit(0);
				} 
				timeinfo = localtime (&rawtime);
				printf("\n-----------------------------\n");
				printf ( "\nMessage receieved at: %s", asctime (timeinfo) );
				printf("Message: %s\n",buffer); 
				// strcat (buffer, asctime (timeinfo));
				printf("Sending echo: %s\n", buffer);
				printf("\n-----------------------------\n");
				if(send(new_socket , buffer , strlen(buffer) , 0 )==-1)
					perror("send"); 
				}
			}
		else{
				close(new_socket);
			} 
		}
	return 0;
}