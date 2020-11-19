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
#include <time.h>

#include <arpa/inet.h>

#define MAXDATASIZE 128 // max number of bytes we can get at once 

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
	const char* PORT = NULL; 
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv, len;
	char s[INET6_ADDRSTRLEN];
	int flag = 1;
	time_t rawtime; 
	struct tm * timeinfo;
	if (argc != 3) {
	    fprintf(stderr,"usage: client IPAdr Port\n");
	    exit(1);
	}
	else 
		PORT = argv[2];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
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
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure
	do{
		printf("\n--------------------------------");
		printf("\nMessage to send: "); 
		if (fgets(buf, MAXDATASIZE-1, stdin) == NULL){
			printf("\n\n********************************");
			printf("\nCtrl + D received..... \nExiting....\n");
			close(sockfd);
			return 0;
		}
		len = ((sizeof(buf))/(sizeof(char)));
		n = send(sockfd, buf, sizeof(buf), 0);
		//strcat(buf, "\n"); //test 2
		if (n == -1)
			perror("send");
		time( &rawtime );
		timeinfo = localtime (&rawtime);
		printf ( "Message sent at: %s", asctime (timeinfo) );
		// exit(0); // test 5
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		time( &rawtime );
		timeinfo = localtime (&rawtime);
		printf ("Echo receieved at: %s", asctime (timeinfo) );
		buf[numbytes] = '\0';
		printf("Message received: %s",buf);
		printf("\n--------------------------------\n\n");
	}while(1);	
		
	return 0;
}

