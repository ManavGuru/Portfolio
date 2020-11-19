#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define MAXDATASIZE 256
#define BACKLOG 10
//SBCP Header Types
#define JOIN    2 
#define FWD     3
#define SEND    4 
#define NAK     5
#define OFFLINE 6
#define ACK     7
#define ONLINE  8
#define IDLE 	9
//************************************************//
//					Variables                     //
//************************************************//

fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number

int listener;     // listening socket descriptor
int newfd;        // newly accept()ed socket descriptor
struct sockaddr_storage remoteaddr; // client address
socklen_t addrlen;

char buf[1000];    // buffer for client data
char buf_send [1000];
char buf_recv [1000];
int nbytes;


char remoteIP[INET6_ADDRSTRLEN];
char msg[512];
char *now;
int yes=1;        // for setsockopt() SO_REUSEADDR, below
int i, j, k, rv, uname_flag =1;

time_t rawtime;
struct tm * timeinfo;
int no_clients = 0; 
int pack_size; 
struct addrinfo hints, *ai, *p;
int connected_cli = 0;
const char * PORT;

char usernames [10][16];
char user_name [16];
char msg_sender [16];

char online_users [512];
//************************************************//
//************************************************//
//					 Helper Functions             //
//************************************************//
struct sbcp_attributes
{
    int16_t type; 
    int16_t length; 
    char * payload;
}recv_atr, send_atr;

struct sbcp_message
{
    int8_t vrsn; 
    int8_t type; // ref line 14
    int16_t length; //length of the sbcp message
    struct sbcp_attributes * payload;
}recv_msg, send_msg;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall ( int s, char *buf, int len){
	int total =0; 
	int bytesleft = len; 
	int n; 
	while (total < len) {
		n = send (s, buf+total, bytesleft, 0); 
		if(n == 1) 
		{ 
			break; 
		}
		total += n; 
		bytesleft -= n; 
	}
	len = total; 
	return n == -1 ? -1:0; 
}

void packi16(char *buf, unsigned int i)
{
*buf++ = i>>8; 
*buf++ = i;
}

int32_t pack(char *buf, char *format, ...)
{
    va_list ap;   //variable argument list 

    int16_t h; //16 bit 

	int8_t c; // 8 bit 

    char *s; // strings 

    int32_t size = 0; 
	int32_t len;

    va_start(ap, format);
    
    for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				size += 2;
				h = (int16_t)va_arg(ap, int); 
				packi16(buf, h);
				buf += 2;
				break;
			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int); 
				*buf++ = (c>>0)&0xff;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = strlen(s);
				size += len + 2;
				packi16(buf, len);
				buf += 2;
				memcpy(buf, s, len);
				buf += len;
				break;
		}
	}
	va_end(ap);
	return size;
}

unsigned int unpacki16(char *buf)
{
	return (buf[0]<<8) | buf[1];
}

void unpack(char *buf, char *format, ...)
{
	va_list ap;  // variable argument list 

	int16_t *h; // 16 bit

	int8_t *c; // 8 bit 

	char *s; //strings

	int32_t len, maxstrlen=0, count;
	va_start(ap, format);
	for(; *format != '\0'; format++) {
		switch(*format) {
            case 'h': // 16-bit
				h = va_arg(ap, int16_t*);
				*h = unpacki16(buf);
				buf += 2;
				break;
			case 'c': // 8-bit
				c = va_arg(ap, int8_t*);
				*c = *buf++;
				break;
			case 's': // string
				s = va_arg(ap, char*);
				len = unpacki16(buf);
				buf += 2;
				if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
				else count = len;
				memcpy(s, buf, count);
				s[count] = '\0';
				buf += len;
				break;
			default:
				if (isdigit(*format)) { 
					maxstrlen = maxstrlen * 10 + (*format-'0');
				}
		}
		if (!isdigit(*format)) maxstrlen = 0;
	}
	va_end(ap);
}

// void function (sbcp_attributes atr, sbcp_message msg, )
//************************************************//
//					 Main Function                //				
//************************************************//

int main(int argc, char const *argv[])
{
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
    
    if (argc != 4) {
        fprintf(stderr, "Usage: server IPaddr Port Client_lim\n");
        exit(1);
    }

    else{
        PORT = argv[2];
        no_clients = atoi(argv[3]);
    }
    	
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        // lose the pesky "address already in use" error message
       	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))!= 0)
       	{
			perror("Server: SetSockOpt");
			exit(1);
       	}

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            perror("Server: Bind Error");
            continue;
        }

        break;
    }
    
    freeaddrinfo(ai); // all done with this

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    printf("server listening......\n");

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                        exit(5);
                    } 
                    else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        	connected_cli+= 1;
                    }
                }
                else 
                {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, 600, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: user %s hung up\n", usernames[i]);
                            //send information to other users 
                            send_atr.payload = usernames [i];
                            send_atr.type = 2;
                            send_atr.length = 20; 
                            send_msg.vrsn = '3'; 
                            send_msg.type = OFFLINE;
                            send_msg.length = 24;
                            send_msg.payload = &send_atr; 
                            // create the SBCP message packet. 
                            pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, 
                            	send_msg.type ,send_msg.length, 
                            	send_atr.type, send_atr.length, send_atr.payload);
	                        for(j = 0; j <= fdmax; j++)
	                        {
	                        	// send to everyone!
	                        	if (FD_ISSET(j, &master))
	                        	{
	                            	// except the listener and ourselves
	                            	if (j != listener && j != i)
	                            	{
	                               	 	if (sendall(j, buf_send, pack_size) == -1)
	                               	 	{
	                                 	   perror("send: notify client exit");
	                                	}
	                            	}
	                        	}
	                    	}
	                    	usernames[i][0] = '\0';
	                    	
                        }
                        else 
                        {
                            perror("recv");
                        }
                        connected_cli-= 1;
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else
                    {
                        // we got some data from a client
                        unpack(buf, "cchhh", &recv_msg.vrsn, &recv_msg.type, &recv_msg.length, 
                        	&recv_atr.type, &recv_atr.length);
                        //check the state of the received message packet. 
                        if(recv_msg.vrsn == '3')
                        {	// new client joing the chat
                        	if(recv_msg.type == JOIN && recv_atr.type == 2)
                        	{
                        		//too many clients  
                        		if(connected_cli > no_clients)
                        		{
                        			//sending out a NAK
                        			sprintf(msg, "Chat room full");
                        			send_atr.payload = msg; 
                        			send_atr.type = 1; 
                        			send_atr.length = 36;
                        			send_msg.vrsn = '3';
                        			send_msg.type = NAK; 
                        			send_msg.length = 40; 
                        			send_msg.payload = &send_atr;
                        			pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.type, 
                        				send_msg.length, send_atr.type, send_atr.length, msg);
                        			if(sendall(i, buf_send, pack_size) == -1)
                        			{
                        				perror("send");
                        			}
                        			connected_cli --; 
                        			close(i);
                        			FD_CLR(i, &master);
                        			break;
                        		}
	                        		
                        		unpack(buf + 8, "s", user_name);
                        		for( j=4; j<=fdmax; j++)
                        		{
                        			if(strcmp(user_name, usernames[j]) == 0)
                        			{
                    					uname_flag = 0; // flag indicating that this username is in use.
                    					sprintf(msg, "Username is already used");
	                        			send_atr.payload = msg; 
	                        			send_atr.type = 1; 
	                        			send_atr.length = 36;
	                        			send_msg.vrsn = '3';
	                        			send_msg.type = NAK; 
	                        			send_msg.length = 40; 
	                        			send_msg.payload = &send_atr;
	                        			pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.type, 
                        				send_msg.length, send_atr.type, send_atr.length, msg);
	                        			if(sendall(i, buf_send, pack_size)== -1)
	                        				perror("send");
	                        			connected_cli --; 
	                        			close(i);
	                        			FD_CLR(i, &master);
	                        			break;
                        			}
                        		}
                        		if (uname_flag == 1)
                        		{
                        			sprintf(usernames[i], "%s", user_name);
                        			printf("Client: %s Connected... \n", usernames[i]);
                        			strcpy (online_users,"Clients in the chat room: ");
                        			for (k = 4; k <= fdmax; k++){
                        				strcat (online_users, usernames[k]);
                        				strcat (online_users , ",");
                        			}
                        			//sending the ACK to the online user
                        			sprintf(msg, "Number of clients: %d\n", connected_cli);
                        			strcat (msg, online_users);
                        			send_atr.payload = msg; 
                        			send_atr.type = 4; 
                        			send_atr.length = 516;
                        			send_msg.vrsn = '3';
                        			send_msg.type = ACK; 
                        			send_msg.length = 520; 
                        			send_msg.payload = &send_atr;
                        			pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.type, 
                    				send_msg.length, send_atr.type, send_atr.length, msg);
                        			if(sendall(i, buf_send, pack_size)== -1)
                        				perror("send: ACK");

                        			//sending ONLINE status message
                        			send_atr.payload = usernames [i];
                            		send_atr.type = 2;
                            		send_atr.length = 16 + 4; 
                            		send_msg.vrsn = '3'; 
                            		send_msg.type = ONLINE;
                            		send_msg.length = 20 + 4;
                           			send_msg.payload = &send_atr; 
                            		// create the SBCP message packet. 
                            		pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.length, 
                            		send_atr.type, send_atr.length, send_atr.payload);
	                        		for(j = 0; j <= fdmax; j++) 
	                        		{
	                        		// send to everyone!
	                        			if (FD_ISSET(j, &master)) 
	                        			{
			                            	// except the listener and ourselves
	    		                        	if (j != listener && j != i) 
	    		                        	{
	            		                   	 	if (sendall(j, buf_send, pack_size) == -1) 
	            		                   	 	{
	                    			             	   perror("send: ONLINE");
	                                			}
	                            			}
	                        			}
	                    			}
                        		}
                    		}

                    		//handling a chat message
                    		if (recv_msg.type == SEND && recv_atr.type == 4)
                    		{	// sending message to connected clients 
                    			unpack(buf + 8, "s", msg_sender);
                    			sprintf(buf_send, "[%s]: %s", usernames[i], msg_sender);
                    			send_atr.payload = buf_send;
                        		send_atr.type = 4;
                        		send_atr.length = 512 + 4; 
                        		send_msg.vrsn = '3'; 
                        		send_msg.type = FWD;
                        		send_msg.length = 516 + 4;
                       			send_msg.payload = &send_atr; 
                        		// create the SBCP message packet. 
                        		pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.length, 
                        		send_atr.type, send_atr.length, buf_send);
                        		for(j = 0; j <= fdmax; j++)
                        		{
                        		// send to everyone!
                        			if (FD_ISSET(j, &master)) 
                        			{
		                            	// except the listener and ourselves
    		                        	if (j != listener && j != i) 
    		                        	{
            		                   	 	if (sendall(j, buf_send, pack_size) == -1) 
            		                   	 	{
                    			             	   perror("send: FWD");
                                			}
                            			}
                        			}
                    			}
                    		}
                    		if(recv_msg.type == IDLE)
                    		{
                				send_atr.payload = usernames[i];
                        		send_atr.type = 2;
                        		send_atr.length = 20; 
                        		send_msg.vrsn = '3'; 
                        		send_msg.type = IDLE;
                        		send_msg.length = 24;
                       			send_msg.payload = &send_atr; 
                        		// create the SBCP message packet. 
                        		pack_size = pack(buf_send, "cchhhs", send_msg.vrsn, send_msg.length, 
                        		send_atr.type, send_atr.length, send_atr.payload);
                        		for(j = 0; j <= fdmax; j++)
                        		{
                        		// send to everyone!
                        			if (FD_ISSET(j, &master)) 
                        			{
		                            	// except the listener and ourselves
    		                        	if (j != listener && j != i) 
    		                        	{
            		                   	 	if (sendall(j, buf_send, pack_size) == -1) 
            		                   	 	{
                    			             	   perror("send: IDLE");
                                			}
                            			}
                        			}
                    			}
                    		}
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}