//ECEN--------------601
//Machine_Problem---4
//Team_Number-------3

#include<sys/types.h>
#include<sys/socket.h>
#define __USE_XOPEN
#include<arpa/inet.h>
#include<netinet/in.h>

#include<netdb.h>
#include<unistd.h>
#include<fcntl.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<time.h>
#include<ctype.h>
int max_cache_capacity; //maximum number of websites cached for faster access
unsigned short int client_num; 

//Declaring structure data types
//structure for http domain and file names
struct Http{
	char File_Name[1024];
	char Host_Name[1024];
};
//struct for cache data
struct Cache_Block{
	char expire_date[100];
	char host_file[200];
	int lastused;
	int current;
	int expire;
};
struct Cache_Block cache[10];

//structure for request logs
struct Condition{
	int expires;
	int length;
	int cb_index;
	int index;
	
	char filename[200];
	bool boolean;
	bool conditional;
};

//intializr cache func
void CACHE_SETUP(){
	for(int j=0; j<10; j++){
		cache[j].lastused=j+1;
		cache[j].current=0;
	}
}

int sock[31]; //array to store client and http server connections
struct Condition req_parameters[30];  
int client_request[30]; //should check working 
int type[30]; //array helping to determine which type of socket is stored in sock
char URL[30][2048]; //URL data
int rr=1; //filename incremator to store cache data

//updating the cache structure
void UPDATE(int block)
{
	if(cache[block].lastused==1)
		return;
	cache[block].lastused = 1;
	for(int i=0; i<10; i++)
	{
		if(i!=block)
		{
			cache[i].lastused++;
			if(cache[i].lastused>10)
				cache[i].lastused = 10;
		}
	}
	return;
}

//LrU Cache updation if all the empty memory is filled
int UPDATE_KICK()
{
	int end = 10;
	int ans = -1;
	while(end>0)
	{
		for(int i=0; i<10; i++)
		{
			if(cache[i].lastused==end && cache[i].current==0)
			{
				
				ans = i;
				cache[ans].current=-2;
				end=0;
				break;
			}
		}
		end--;
	}
	return ans;
}



// Check expire information
bool CALC_EXPIRE_BOOL(int cacheBlock_index)
{
	time_t raw_time;
	time(&raw_time);
	struct tm * utc;
	utc = gmtime(&raw_time);
	raw_time = mktime(utc);
	if(cache[cacheBlock_index].expire-raw_time>0)
		return false;
	return true;
}

// Checking if webpage already in cache
int CHECKCACHE(char *req)
{
	for(int s=0;s<10;s++){
		if(strcmp(req,cache[s].host_file) ==0){
			return s;
		}
	}	
	return -1;
}

// Parsing the data to determine host name and file name
struct Http * parse_http_request(char *buf, int bytes_num)
{
	struct Http* out = (struct Http *)malloc(sizeof(struct Http));
	int temp;
	
	for(temp=0; temp<bytes_num; temp++)
	{
		if(buf[temp]==' ')
		{
			temp++;
			break;
		}
	}
	int j=0;
	out->File_Name[0]='/';
	
	if(buf[temp+8]=='\r' && buf[temp+9]=='\n')
	{
		out->File_Name[1]='\0';
	}
	else
	{
		while(buf[temp]!=' ')
			out->File_Name[j++]=buf[temp++];
		out->File_Name[j]='\0';
	}
	temp+=1;
	while(buf[temp++]!=' ');
	
	j=0;
	
	while(buf[temp]!='\r')
		out->Host_Name[j++]=buf[temp++];
	
	out->Host_Name[j]='\0';
	
return out;
}

int main(int argc, char *argv[])
{
       //intializing all the arrays
	for(int p=0;p<30;p++){
	     sock[p]=-1;
	     type[p]=-1;
	     client_request[p]=-1;
	}	
	max_cache_capacity = 10; // max cache stored is 10. If exceeds replaces using LRU scheme
	client_num = 0;
	int sock_fd, error;
	int i, bytes_num;
	char buffer[2048],TB[256],tmp[512];

	struct addrinfo hints, *server_info, *p;
	//checking input args
	if(argc!=3)
	{
		fprintf(stderr,"Invalid number of arguments : server ip port\n");
		return 1;
	}
	
	// searching and bind the socket given as input
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if((error = getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		exit(1);
	}
	
	for(p=server_info; p!=NULL; p=p->ai_next)
	{
		
		if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
			perror("server: socket error");
			continue;
		}
		
		if((error = bind(sock_fd, p->ai_addr, p->ai_addrlen))== -1){
			perror("bind error\n");
			continue;
		}
		break;
	}
	// Binding error
	if(p==NULL){
		fprintf(stderr, "Bind failed\n");
		return 2;
	}
	//starting to listen for incoming connections										
	if( (error = listen(sock_fd, max_cache_capacity)) == -1){
		perror("server: listen failed");
	}
	
	
	freeaddrinfo(server_info);
	//storing the listen socket in array
	sock[30]=sock_fd;	
	//declaring and setting variables for I/O multiplexing
	fd_set master, read_fds, write_fds, master_write;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&master_write);
	FD_SET(sock_fd,&master);
	
	int fdmax = sock_fd, c_sockfd;
	struct sockaddr_storage client_addr;
	socklen_t addr_length;
	
	//Intialize cache
	CACHE_SETUP();
	
	
	struct tm tm,*utc;
	time_t rawtime;
	
	printf("HTTP Proxy\n*********************************\n");
	
	
	while(1)
	{
	   // keeping a master fd not altered by select
	   read_fds =  master;
       	   write_fds = master_write;
	   
	   //using select 
		if( (error = select(fdmax+1, &read_fds, &write_fds, NULL, NULL)==-1)){
			perror("Server: Select");
			exit(4);
		}	   
			
		for(i=0; i<=30; i++)
		{ //looing to check the stored socket descriptors
		   if(sock[i]==-1) // array space not used yet
			   continue;
		// Using write fds to write the retrieved information stored in cache to user
		   if(FD_ISSET(sock[i],&write_fds))
		    {			   
				if(FD_ISSET(sock[i],&master)==false) //checking for any error
				{
				   	FD_CLR(sock[i],&master);
					FD_CLR(sock[i],&master_write);
					sock[i]=-1;
					close(sock[i]);
					
					//setting back variables 
					if(req_parameters[i].cb_index==-1)
					{
						if(req_parameters[i].boolean==true)
								strcpy(req_parameters[i].filename,"");	
					}
							
					else if(req_parameters[i].cb_index != -1)
					{
						UPDATE(req_parameters[i].cb_index);
						cache[req_parameters[i].cb_index].current-=1;
					}
					continue;
			    	}
				
				//Opening to read the cache data
				FILE *first_file;
				first_file=fopen(req_parameters[i].filename,"r");
				// file opening error
				if(!first_file)
				{
					printf("%s\n",req_parameters[i].filename);
					FD_CLR(i,&master_write);
					printf("Unable to Open Cache File\n");
				}
				//The select conditions make to loop tell all the data is transmitted to the user
				else
				{
					fseek(first_file,(req_parameters[i].index)*2048,SEEK_SET);		
					req_parameters[i].index+=1;
					bytes_num=fread(buffer,1,2048,first_file);
                    			if(bytes_num!=0)
					{
						if( (error = send(sock[i],buffer,bytes_num,0)) == -1)
						{			
								perror("Client send error");
						}
					}
					
					// All the data requested has transmitted. So closing the write and read ports with user
					if(bytes_num<2048)
					{						
						FD_CLR(sock[i],&master_write);
						FD_CLR(sock[i],&master);			
						close(sock[i]);
						sock[i]=-1;
						
						printf("*********************************\n");
						
						if(req_parameters[i].cb_index==-1)
						{
							if(req_parameters[i].boolean==true)		
								strcpy(req_parameters[i].filename,"");	
						}
						else if(req_parameters[i].cb_index != -1)
						{
							UPDATE(req_parameters[i].cb_index);	
							cache[req_parameters[i].cb_index].current-=1;		
						}
					}	
					
				}
				//closing the file
				fclose(first_file);
			}
            else if(FD_ISSET(sock[i],&read_fds))
			{
				
				// accepting the new clients till the server resources are present
				if(sock[i]==sock_fd)
				{
					
					addr_length = sizeof(client_addr);		
					
					if( (c_sockfd = accept(sock_fd, (struct sockaddr *)&client_addr,&addr_length)) == -1)
					{
						perror("Accept error");
					}
					else
					{
						FD_SET(c_sockfd,&master);
						for(int s=0;s<=30;s++){
							if(s==30){
								printf("maximum connection sockets reached. closing the tcp connection\n");
								close(c_sockfd);
								break;
							}
							if(sock[s]==-1){// storing the user socketid in array
								sock[s]=c_sockfd;
								type[s]=0;
								break;
							}

						}
						client_num++; 
						if(c_sockfd > fdmax)
							fdmax = c_sockfd;
						printf("Connection established\n");
					}
				}
				else // data received from HTTPs server or client 
				{
				   	if((bytes_num = recv(sock[i],buffer,sizeof(buffer),0)) <=0)
					{//either reading data from HTTP server is done or error
						    if(bytes_num<0){
							perror("recv");
						     }	
						    //closing the HTTP server connection
							close(sock[i]);
							FD_CLR(sock[i],&master);
							sock[i]=-1;
						       //type---0 expecting message from user
						       //type----1 message from HTTP server						
							if(type[i]==1)
							{
									
									FD_SET(sock[client_request[i]],&master_write); // setting the write_fd to transfer cached data to user
									int j = client_request[i];
									int a = req_parameters[j].cb_index;
									if(a==-1){
									}
									else
									{//checking the cached data to retrieve expire data and other information
									
										FILE *third_file;
										third_file=fopen(req_parameters[j].filename,"r");
										
										char *line=NULL;
										int line_size=0,line_length;
										bool boo=false;
										
										if(third_file)
										{
											line_length=getline(&line,&line_size,third_file);
											// checking if cache is changed
											
											if(strstr(line,"304")!=NULL)		
												boo = true;
											bool flag = false;
											//reading line by line and checking
											while(line_length>=0)
											{
												line_length=getline(&line,&line_size,third_file);
												
												if(strcmp(line,"\r") == 0)
												{
													break;
												}
												
												for(int q=0;q<line_size;q++){
													line[q]=tolower(line[q]);
												}
												
												//Determing expiry time
												if(strstr(line,"expires:")==line)
												{
													flag = true;
													int pos = 8;
													
													if(line[pos]==' ')
														pos++;
													
													strcpy(cache[a].expire_date,line+pos);
													memset(&tm, 0, sizeof(struct tm));
													
													strptime((line+pos), "%a, %d %b %Y %H:%M:%S ", &tm);
													cache[a].expire = mktime(&tm);
													break;
												}
											}
											fclose(third_file);
											free(line);
											//freeing the line memory
											
											if(!flag)
											{
												
												time(&rawtime);
												utc = gmtime(&rawtime);
												
												strftime(TB, sizeof(TB), "%a, %d %b %Y %H:%M:%S ",utc);	
												cache[a].expire = mktime(utc);
												strcat(TB,"GMT");
												strcpy(cache[a].expire_date,TB);
											}
										}
										
										time(&rawtime);
										utc = gmtime(&rawtime);
										strftime(TB, sizeof(TB), "%a, %d %b %Y %H:%M:%S ",utc);
										printf("Access Time:%s GMT \n",TB);
										printf("File Expires at: %s\n",cache[a].expire_date);

										if(boo==true)
										{
											//sending data from cached file
											printf("Retrieved from cache\n");
											
											if(req_parameters[j].conditional)
											{
											
												char s4[20];
												sprintf(s4,"%d",a);
												strcpy(tmp,s4);
												strcpy(req_parameters[j].filename,tmp);									
												req_parameters[j].boolean = false;
											}
										}

										if(boo==false)
										{
											
											if(cache[a].current!=1)
											{
												if(cache[a].current>1)
												{
													cache[a].current-=1;
													int new_cb;
													new_cb = UPDATE_KICK();
													
													if(new_cb==-1)
													{
														req_parameters[j].boolean = true;
														req_parameters[j].cb_index = new_cb;
														a = new_cb;
													}
													else
													{
														req_parameters[j].cb_index = new_cb;
														cache[new_cb].expire = cache[a].expire;
														strcpy(cache[new_cb].expire_date,cache[a].expire_date);
														a = new_cb;
													}
												}
											}
											else
											{
												cache[a].current = -2;
											}
										}
										//update cache 
										if(a!=-1 && cache[a].current==-2)
										{
											char s3[20];
											sprintf(s3,"%d",a);
											strcpy(tmp,s3);	
											strcpy(req_parameters[j].filename,tmp);
											
											strcpy(cache[a].host_file,URL[client_request[i]]);
											req_parameters[j].boolean = false;
											cache[a].current = 1;
										}
									}
						    
							}
							//data from client=0 indicating termination from client
							else
							{
								printf("Client disconnected\n");		
						    }
								
					}
					//data from HTTP server
					else if(type[i]==1) //writing the read data from HTTP server to cache file
					{
						FILE *tmp_file;
						tmp_file=fopen(req_parameters[client_request[i]].filename,"a+");
						
						if(!tmp_file)
						{ 
							printf("File not found!!\n");
						}
						else
						{
							fwrite(buffer,1,bytes_num,tmp_file);
							fclose(tmp_file);
						}
					}
					
					else if(type[i]==0)
					{ // Intial message from cliemt giving the Url
						struct Http* tmp;
						//parsing the url
						tmp = parse_http_request(buffer,bytes_num);
					        strcpy(URL[i],tmp->Host_Name);
						strcat(URL[i],tmp->File_Name);
					//creating HTTP request	
						printf("URL requested: %s\n",URL[i]);
				//checking if webpage already in cache
						int cacheBlock_index = CHECKCACHE(URL[i]);
						bool expired = false;
						req_parameters[i].conditional = false;
						
						if(cacheBlock_index!=-1)//data in cache
						{
							if(cache[cacheBlock_index].current>=0)
							{
								//check expiration
								expired = CALC_EXPIRE_BOOL(cacheBlock_index);
								
								if(expired)
								{
									req_parameters[i].conditional = true;
								}
								
								else
								{	//setting variables to send data from cache
									req_parameters[i].cb_index = cacheBlock_index;		
									req_parameters[i].index = 0;
									req_parameters[i].boolean = false;
									
									cache[cacheBlock_index].current += 1;
									
									char s18[10];
									sprintf(s18,"%d",cacheBlock_index);
									
									strcpy(req_parameters[i].filename,s18);
									FD_SET(i,&master_write);
									
									printf("Cache Hit at  : %d\n",cacheBlock_index);
									continue;
								}
							}
						}
						if(cacheBlock_index==-1 || expired)
						{ //cache expired or not present
							//creating HTTP request to the server
							char tmp_str[2048] = "GET ";
							strcat(tmp_str,tmp->File_Name);
							char s10[]=" HTTP/1.0\r\nHost: ";
							strcat(tmp_str,s10);
							strcat(tmp_str,tmp->Host_Name);
							char s11[]="\r\n\r\n";
							strcat(tmp_str,s11);
							bytes_num = strlen(tmp_str);
							strcpy(buffer,tmp_str);
							
							if(!expired)
							{
								cacheBlock_index = UPDATE_KICK();
								
								printf("Cache Miss\nCache Memory Assigned!\nBlock Number: %d\n" ,cacheBlock_index);
							}
							else
							{
								cache[cacheBlock_index].current+=1;
								buffer[bytes_num-2]='\0';
								
								// Incase the Request is stale, will send a GET request conataining If-modified-since in header 

								char new_req[2048]="";
								strcpy(new_req,buffer);
								char s13[]="If-Modified-Since: ";
								strcat(new_req,s13);
								strcat(new_req,cache[cacheBlock_index].expire_date);
							       	char s14[]="\r\n\r\n\0";
								strcat(new_req,s14);
								bytes_num=strlen(new_req);
								strcpy(buffer,new_req);
								printf("Cache Hit at Block %d\n", cacheBlock_index);
								printf("If-Modified-Since: %s\n",cache[cacheBlock_index].expire_date);
								//cout<<"Status 304 :Served from proxy because of IF-modified"<<endl;
							}
							req_parameters[i].cb_index = cacheBlock_index;
							
							
							int new_socket;
							memset(&hints,0,sizeof(hints));		
							hints.ai_family = AF_INET;
							hints.ai_socktype = SOCK_STREAM;
							// Getting the server address
							if((error = getaddrinfo(tmp->Host_Name, "80", &hints, &server_info)) != 0)
							{	
								fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
								exit(1);
							}
							for(p=server_info; p!=NULL; p=p->ai_next)
							{
								if((new_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
								{ //new socket for HTTP server
									perror("socket fail");
									continue;
								}
								break;
							}
							if(p==NULL)
							{
								fprintf(stderr, "bind error\n");
								return 2;
							}
							if( (error = connect(new_socket,p->ai_addr,p->ai_addrlen)) == -1)
							{
								perror("connect error\n");
							}	
							freeaddrinfo(server_info);
						
							for(int s=0;s<=30;s++){
								if(s==30){ // adding the HTTP server_fd to list
									printf("maximum connection sockets reached. closing the tcp connection\n");
									close(new_socket);
									break;
								}
							if(sock[s]==-1){
								sock[s]=new_socket;
								type[s]=1;//making type=1 as next data should be received from HTTP server
								client_request[s]=i;
								break;
							}

						}
							
							
							req_parameters[i].boolean = true;				
							req_parameters[i].index = 0;
                            
							// Incrementing the variable to create different files for storing cache
							rr++;
							char s15[ ]="tmp_";
							char s16[10];
							sprintf(s16,"%d",rr);
							strcpy(req_parameters[i].filename,"");
							strcpy(req_parameters[i].filename,s15);
							strcat(req_parameters[i].filename,s16);
							FD_SET(new_socket,&master); // setting to the newsocket to checkfor incoming data
							if(new_socket > fdmax)
								fdmax = new_socket;
							if( (error = send(new_socket,buffer,bytes_num,0)) == -1)
							{
								
								perror("send error");
							}
						}
						free(tmp);
					}	
				}	
			}	
		}
	}
		
return 0;	
}
