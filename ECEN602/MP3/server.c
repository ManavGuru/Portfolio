
//-------------------TFTP SERVER TEAM 3-----------------//
//                                                      //
//                  Naga Prashant                       //
//                  Manav Gurumoorthy                   //
//------------------------------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PAYLOAD_MAX 512

#define RRQ      1
#define WRQ      2 
#define DATA     3
#define ACK      4
#define ERROR    5

int readable_timeo(int FD, int t_sec) {
  fd_set rset;
  struct timeval tv;

  FD_ZERO(&rset);
  FD_SET (FD, &rset);

  tv.tv_sec = t_sec;
  tv.tv_usec = 0;

  return (select (FD + 1, &rset, NULL, NULL, &tv));
}

int main(int argc, char *argv[])
{
  //Declaring variables
  char client_address[INET6_ADDRSTRLEN];
  char data[PAYLOAD_MAX] = {0};
  int server_fd, newsock_fd;
  struct sockaddr_in client;
  socklen_t clientlen = sizeof(struct sockaddr_in);
  int g, ret;
  int send_res;
  int last_block;
  if (argc != 3){
    perror("Invalid arguments.\nVALID ARGUMENTS ./server IP Port");
    return 0;
  }
  int  recv_byte;
  char buffer [1024] = {0};
  char ack[32] = {0};
  char tftp_pl[PAYLOAD_MAX+4] = {0};
  char tftp_pl_temp[PAYLOAD_MAX+4] = {0};
  char f_name[PAYLOAD_MAX];
  char Mode[PAYLOAD_MAX];
  unsigned short int opcode1, opcode2, block_number;
  int i, j;
  FILE *fp;
  struct addrinfo *ai;
  struct sockaddr_in server_address; 
  
  int pid;
  int retreive_read;
  int number_block = 0;
  int count_timeo = 0;
  int ack_no = 0;
  int server_port=htons(atoi(argv[2]));
  //creating socket for the port number specified as argument initially
  memset(&server_address,0,sizeof(server_address));
  if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)

        {
                perror("socket creation failed");
                exit(EXIT_FAILURE);
        }

        // Forcefully attaching socket to the port given as argument 
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&server_port, sizeof(server_port)))
        {
                perror("setsockopt failed");
                exit(EXIT_FAILURE);
        }
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = server_port;

        // binding the socket 
        if (bind(server_fd, (struct sockaddr *)&server_address,sizeof(server_address))<0)
        {
                perror("binding error");
                exit(EXIT_FAILURE);
        }
  printf("SFTP_Server: socket binded\nlooking for clients...\n");
  //start receiving requests from clients
  while(1) {
    recv_byte = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &clientlen);
    if (recv_byte < 0) {
      perror("ERR: error in receiving first packet from client");
      exit(1);
      return 7;
    }
    //received a request from client and checking the opcode to determine further procedure
    printf("SFTP_Server:receiving data from client with  IP address %s\n", inet_ntop(AF_INET, &client.sin_addr,client_address, sizeof(client_address)));       
    memcpy(&opcode1, &buffer,2);                    
    opcode1 = ntohs(opcode1);
    //creating a child process to respond to client request
    pid = fork();
    if (pid == 0) {   //Child process servicing the client 
      if (opcode1 == RRQ){                            //processing read request from client
        memset(f_name,0,PAYLOAD_MAX);
        i = 0;
        while (buffer[2+i] != '\0') {        // looping till the end of file
          f_name[i]=buffer[2+i];
          i++;
        }	
        f_name[i]='\0';                       
        memset(Mode,0,PAYLOAD_MAX);
        g = 0;
        j = i+ 3;
        while( buffer[j] != '\0'){    
          Mode[g]=buffer[j];
          g++;
          j++;
        }	
        Mode[g]='\0';
        printf("SFTP_Server:received read request for file: %s in: %s mode\n", f_name, Mode);
        fp = fopen (f_name, "r"); //opening file requested
        if (fp != NULL) {          //checking if the file name is valid
        close(server_fd);	//closing the SFTP_Server socket
	 
        //creating a new child socket to communicate with client
	     struct sockaddr_in child_address; 
	     memset(&child_address,0,sizeof(child_address));
	     if((newsock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
         {
                 perror("new socket failed");
                 exit(EXIT_FAILURE);
         }
         child_address.sin_family = AF_INET;
         child_address.sin_addr.s_addr = INADDR_ANY;
         child_address.sin_port = htons(0);
         if (bind(newsock_fd, (struct sockaddr *)&child_address,sizeof(child_address))<0)
         {
                 perror("binding error");
                 exit(EXIT_FAILURE);
         }

          // creating data packet 
          memset(tftp_pl,0, sizeof(tftp_pl));
          memset(data,0, sizeof(data));
          //reading 512 bytes data from file
          retreive_read = fread (&data,1,PAYLOAD_MAX,fp);
          if(retreive_read>=0) {
            data[retreive_read]='\0';
          }
          if(retreive_read < PAYLOAD_MAX) //checking data length to determine last packet
            last_block = 1;
          block_number = htons(1);                            // first data packet
          opcode2 = htons(DATA);                                // Opcode = 3 meaning data packet
          memcpy(&tftp_pl[0], &opcode2, 2);             // inserting opcode in the first two bytes
          memcpy(&tftp_pl[2], &block_number, 2);         // inserting block/sequence number
 
          while ( data[i] != '\0') {
            i++;            //copying data from the file
            tftp_pl[i+4] = data[i];
          }
          memset(tftp_pl_temp,0, sizeof(tftp_pl_temp));
          memcpy(&tftp_pl_temp[0], &tftp_pl[0], 516);
	  //sending data to client
          send_res=sendto(newsock_fd, tftp_pl, (retreive_read + 4), 0, (struct sockaddr*)&client, clientlen);
          ack_no = 1;
          if (send_res < 0){
            perror("failed to send first packet ");
	    exit(1);
	  }
          printf("SFTP_Server:first packet sent\n");
	  //looping to send other packets till end of file transfer
          while(1){
	    //waiting for timeout/ack
            if (readable_timeo (newsock_fd, 1) != 0) {
	      //receive message from client checking if ack is proper
              memset(buffer,0, sizeof(buffer));
              memset(tftp_pl,0, sizeof(tftp_pl));
              recv_byte = recvfrom(newsock_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &clientlen);
              count_timeo = 0;        
              if (recv_byte < 0) {
                perror("Didn't receive data\n");
		            exit(1);
                return 6;
              }
	      inet_ntop(AF_INET, &client.sin_addr,client_address, sizeof(client_address));
              printf("SFTP_Server:received %d bytes of data ", recv_byte);
              printf("from client with IP address %s\n", client_address);
              memcpy(&opcode1, &buffer[0], 2);
              if (ntohs(opcode1) == ACK) {                               // checking opcode for ack
                memset(&number_block,0, sizeof(number_block));
                memcpy(&number_block, &buffer[2], 2);
                number_block = ntohs(number_block);
                if(number_block == ack_no) {                     // checking the block/sequence no. of ack
                  printf("ACK  received\n");
                  ack_no = (ack_no + 1)%65536;
          	if(last_block == 1){			//exiting if its last block
                    close(newsock_fd);
                    fclose(fp);
          	  printf("SFTP_Server: File transmission completed. Closing the socket\n");
          	  exit(5);
          	  last_block = 0;
          	}
          	else {          //tranmitting next data packet
                    memset(data,0, sizeof(data));
                    retreive_read = fread (&data,1,PAYLOAD_MAX,fp);           // Reading data from file
                    if(retreive_read>=0) {
                      if(retreive_read < PAYLOAD_MAX)
                        last_block = 1;
                      data[retreive_read]='\0';
                      block_number = htons(((number_block+1)%65536));              // wrapping block number
                      opcode2 = htons(DATA);
                      memcpy(&tftp_pl[0], &opcode2, 2);
                      memcpy(&tftp_pl[2], &block_number, 2); 
                      for (i = 0; data[i] != '\0'; i++) {        
                        tftp_pl[i+4] = data[i];		//reading data from file
                      }	                       
                      memset(tftp_pl_temp,0,sizeof(tftp_pl_temp));
                      memcpy(&tftp_pl_temp[0], &tftp_pl[0], 516);
		      //sending data packet to client
                      int send_res = sendto(newsock_fd, tftp_pl, (retreive_read + 4), 0, (struct sockaddr*)&client, clientlen);
                      if (send_res < 0){ //checking any error in sending
                        perror("ERR: Sending data ");
			exit(1);
		      }
                    }
                  }
                }
                else { 
                  printf("received ack for different packet: ack_no: %d, blockNo: %d\n", ack_no, number_block);
                }
              }
            }
            else {
	      //time out occured. So retransmitting
              count_timeo++;
              printf("Timeout occurred.\n");
              if (count_timeo == 10){ //checking if client terminated inbetween
                printf("Closing client as 10 timeouts occured\n");
                close(newsock_fd);
                fclose(fp);
                exit(6);
              }
              else { 
		//creating data packet
                memset(tftp_pl,0,sizeof(tftp_pl));
                memcpy(&tftp_pl[0], &tftp_pl_temp[0], 516);
                memcpy(&block_number, &tftp_pl[2], 2);
                block_number = htons(block_number);
                printf ("SFTP_Server:retransmitting the data packet: %d\n", block_number);
		//sending data again
                send_res = sendto(newsock_fd, tftp_pl_temp, (retreive_read + 4), 0, (struct sockaddr*)&client, clientlen);
                memset(tftp_pl_temp,0,sizeof(tftp_pl_temp));
                memcpy(&tftp_pl_temp[0], &tftp_pl[0], 516);
                if (send_res < 0){ 
                  perror("ERR: Sending Data ");
		  exit(1);
		}
              } 
            }
          }
        }
        else {                                             // sending file not found error message
          unsigned short int ERRCode = htons(1);
          unsigned short int ERRoc = htons(ERROR);             // opcode for error
          char ERRMsg[PAYLOAD_MAX] = "File not found";
          char ERRBuff[PAYLOAD_MAX+4] = {0};
          memcpy(&ERRBuff[0], &ERRoc, 2);
          memcpy(&ERRBuff[2], &ERRCode, 2);
          memcpy(&ERRBuff[4], &ERRMsg, PAYLOAD_MAX);
          sendto(server_fd, ERRBuff, 516, 0, (struct sockaddr*)&client, clientlen);      
          printf("file does not exit. Deallocating resource\n");
          close(server_fd);
          fclose(fp);
          exit(4);
        }
      }
      else if (opcode1 == WRQ) {                                 // Bonus feature WRQ 
	//Creating a child socket
	struct sockaddr_in child_address; 
	memset(&child_address,0,sizeof(child_address));
	if((newsock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
        {
                perror("new socket failed");
                exit(EXIT_FAILURE);
        }
        child_address.sin_family = AF_INET;
        child_address.sin_addr.s_addr = INADDR_ANY;
        child_address.sin_port = htons(0);
        if (bind(newsock_fd, (struct sockaddr *)&child_address,sizeof(child_address))<0)
        {
                perror("binding error");
                exit(EXIT_FAILURE);
        }
	printf("SFTP_Server: received Write Request(WRQ) from client.\n");
	FILE *fp_wr = fopen("data.txt", "w+");
	if (fp_wr == NULL)
	  printf("SFTP_Server: Problem in opening file for WRQ request");
        opcode2 = htons(ACK);
	block_number = htons (0);
        memset(ack,0,sizeof(ack));
        memcpy(&ack[0], &opcode2, 2);
        memcpy(&ack[2], &block_number, 2);
        send_res = sendto(newsock_fd, ack, 4, 0, (struct sockaddr*)&client, clientlen);         
	//sending ack indicating SFTP_Server ready
	ack_no = 1;
        if (send_res < 0){ 
          perror("WRQ ACK ERR: Error sending ACK packet ");
	  exit(0);
	}
	while(1){
          memset(buffer,0,sizeof(buffer));
	  //reading data from socket and writing in file
          recv_byte = recvfrom(newsock_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &clientlen);
          if (recv_byte < 0) {
            perror("WRQ: Error in data reception\n");
	    exit(1);
            return 9;
          }
          memset(data,0,sizeof(data));
          memcpy(&block_number, &buffer[2], 2);
	  g = 0;
          for (i = 0; buffer[i+4] != '\0'; i++) {        
 	    if (buffer[i+4] == '\n'){
	      printf("LF character spotted.\n");
	      g++;
	      if (i-g<0)
	        printf("SFTP_Server: ERROR: i-g is less than 0");
	      data[i-g] = '\n';
            }	      
	    else
              data[i - g] = buffer[i+4];
          }	
	  fwrite(data, 1, (recv_byte - 4 - g), fp_wr);
	  block_number = ntohs(block_number);
	  if (ack_no == block_number){
	    printf("SFTP_Server: Received data block %d as expected\n", ack_no);
            opcode2 = htons(ACK);
	    block_number = ntohs(ack_no);
            memset(ack,0,sizeof(ack));
            memcpy(&ack[0], &opcode2, 2);
            memcpy(&ack[2], &block_number, 2);
	    printf("SFTP_Server: Sending ack for block %d\n", htons(block_number));
            send_res = sendto(newsock_fd, ack, 4, 0, (struct sockaddr*)&client, clientlen);    
   	    //terminating if less than 516 bytes are received	    
	    if (recv_byte < 516) {
              printf("File reception completed. Closing the client socket\n");
              close(newsock_fd);
              fclose(fp_wr);
              exit(9);
	    }
            ack_no = (ack_no + 1)%65536;
	  }
	}
      }
    }
  }  
}
							