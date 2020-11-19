/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include<sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include "sbcp.h"
 
#define STDIN_DESCRIPTOR 0

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	int total = 0, n;
	int bytesleft; 
	const char* PORT = NULL; 
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv, len;
	char s[INET6_ADDRSTRLEN];
	int flag = 1;
	time_t rawtime; 
	struct tm * timeinfo;
	//checking the input arguments
	if (argc != 4) {
	    fprintf(stderr,"usage: client IPAdr Port username\n");
	    exit(1);
	}
	else 
		PORT = argv[2];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//getting the IP address from host name
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		//sending the connection creation
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	//printing the IP address of connected server 
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	struct sbcp_message user;
	strcpy(user.attribute.payload,argv[3]);
	user.version=3;
	user.type=2;
	user.length=24;
	user.attribute.length=20;
	user.attribute.type=2;
	n = send(sockfd,&user, sizeof(user), 0);
	if (n == -1)
		perror("send");


	fd_set readfds;
	while(1){
		FD_SET(STDIN_DESCRIPTOR,&readfds);
		FD_SET(sockfd,&readfds);
		if(select(sockfd+1,&readfds,NULL,NULL,NULL)<0){
			fprintf(stderr,"select error");
			exit(0);
		}

		if(FD_ISSET(STDIN_DESCRIPTOR,&readfds)){
			printf("reading message from stdin.");
			//			//printf("\nMessage to send: "); 
			//reading the message from standard input
			if (fgets(buf, MAXDATASIZE-1, stdin) == NULL){
				printf("\n\n********************************");
				printf("\nCtrl + D received..... \nExiting....\n");
				close(sockfd);
				return 0;
			}
			struct sbcp_message message;
			strcpy(message.attribute.payload,buf);
			message.version=3;
			message.type=4;
			message.length=520;
			message.attribute.type=4;
			message.attribute.length=524;
			n = send(sockfd,&message, sizeof(message), 0);
			if (n == -1)
				perror("send");
			printf("message sent\n");
			printf("\n--------------------------------\n");
			//FD_SET(STDIN_DESCRIPTOR,&readfds);
		}
		if(FD_ISSET(sockfd,&readfds)){
			struct sbcp_message message;
		//reading the forwarded message from server
			if ((numbytes = recv(sockfd,&message,sizeof(message), 0)) == -1) {
		    	perror("recv");
		    	exit(1);
			}
		/*
		time( &rawtime );
		timeinfo = localtime (&rawtime);
		printf ("Echo receieved at: %s", asctime (timeinfo) );
		buf[numbytes] = '\0';
		*/   
			printf("%s\n",message.attribute.payload);
			printf("\n--------------------------------\n\n");
			//printf("hihihihi");
		}
		//printf("exiting if JWUJGW");
	};	
		
	return 0;
}

