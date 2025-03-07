#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h> 
#include <linux/fs.h>

#define GOOD_VAL 0
#define ERR_VAL -1

#define RECV_BUFFER 512
#define SOCKET_BACKLOG 100

/* Returns -1 if any of the steps setting the socket fails */

int socket_creation(int * sock_fd, struct sockaddr * client_addr)
{
	int ret = GOOD_VAL;
	
	//Get server fd socket();
	*sock_fd = socket(PF_INET, SOCK_STREAM, 0); //SOCK_STREAM for TCP. Which is a reliable two way connection that incides in having the packets automatically retried when droped contrarily to SOCK_DGRAM (Datagram socket)(UDP --> Which has more emphasys in latency than in delivery, so it can have packets droped (commonly used in video streams))
	if(*sock_fd<0){
		ret = ERR_VAL;
		perror("socket error");
	}
	else{
		//Bind with bind() using getaddrinfo() for socket addr info
		struct addrinfo hints; //hints for addr creation
		struct addrinfo *dc_addrinfo; //Will point to the results
		
		memset(&hints, 0, sizeof hints); //Ensure everything is empty for us to properly fill
		hints.ai_flags = AI_PASSIVE; //Fill IP for me
		hints.ai_family = AF_INET; //Address family IPV4
		hints.ai_socktype = SOCK_STREAM; //TCP stream socket
		
		if(getaddrinfo(NULL, "9000", &hints, &dc_addrinfo) != 0){
			ret = ERR_VAL;
			perror("getaddrinfo error");
		}
		else
		{
			ret = bind(*sock_fd, dc_addrinfo->ai_addr, sizeof(*(dc_addrinfo->ai_addr)));
			if(ret<0){
				perror("Bind error");
			}		
			free(dc_addrinfo); //free malloc for addrinfo	
		}
	}	
	if(ret!=ERR_VAL){
		//Listen listen() for new connections (Backlog to determine how many connections are allowed)
		ret = listen(*sock_fd,SOCKET_BACKLOG); //Accepts up to 1 connection?
		if(ret<0){
			perror("Listen error");
		}		
	}

	return ret;
}

int socket_connection(int sockfd, int * client_sock, struct sockaddr * client_addr)
{
	int ret = GOOD_VAL;
	socklen_t client_len = sizeof(*client_addr);
	
	//Accept accept() which will return a new fd for the established socket connection	
	ret = accept(sockfd, client_addr, &client_len);
	if(ret!=ERR_VAL){ //If the call succeeds, save sockfd
		*client_sock = ret;
		syslog(LOG_INFO, "Accepted connection from %s", client_addr->sa_data);
		ret = GOOD_VAL;
	}
	else{
		perror("Socket accept error");
	}

	return ret;
}

int return_data_to_client(int file_fd, int sock_fd)
{
	int ret = GOOD_VAL;
	//char read_buff[256];
	
	char *send_string = malloc(RECV_BUFFER);
	memset(send_string,'\0',RECV_BUFFER); //Add null everywhere
	ssize_t current_size = RECV_BUFFER;
	ssize_t bytes_read_total = 0;
	ssize_t bytes_read = 0;
	
	//Read data from file
	lseek(file_fd, 0, SEEK_SET); // Move to the beginning of the file
	while((bytes_read = read(file_fd, send_string + bytes_read_total, RECV_BUFFER)) > 0){ //0 indicates end of file
		bytes_read_total += bytes_read;
		if(bytes_read_total >= current_size){
			ssize_t new_size = current_size + RECV_BUFFER;
			char *aux_buffer = realloc(send_string, new_size);
			if(!aux_buffer){
				perror("Error reallocating read size");
				free(send_string);
				ret = ERR_VAL;
				return ret;
			}
			current_size = new_size;
			send_string = aux_buffer;
		}	
	}
	//Once the file is completely read, send:
	//printf("send: %s,\n\n bytes_read_total: %ld\n\n", send_string, bytes_read_total);
	int sent_bytes = send(sock_fd, send_string, bytes_read_total, 0);
	free(send_string);
	if(sent_bytes<0){
		perror("Error sending data to client");
		return sent_bytes;
	}
	return ret;
}

bool aesdsock_close_gracefully = false; //global variable

static void signal_handler(int signal_number)
{
	if((signal_number == SIGINT)||(signal_number == SIGTERM)){
		aesdsock_close_gracefully = true;
		syslog(LOG_INFO, "Caught signal, exiting");
	}
}

int main(int argc, char **argv)
{
	int ret = GOOD_VAL;
	int sockfd = 0;
	int client_sock = 0;
		
	//Open thread to manage signals
	
	struct sigaction closing_gracefully_action;
	memset(&closing_gracefully_action, 0, sizeof(struct sigaction));
	closing_gracefully_action.sa_handler=signal_handler; //Associate handler function
	if(sigaction(SIGTERM, &closing_gracefully_action, NULL) != 0){ //Subscribe to SIGTERM calls
		ret = ERR_VAL;
	}
	if(sigaction(SIGINT, &closing_gracefully_action, NULL) != 0){ //Subscribe to SIGINT calls
		ret = ERR_VAL;
	}
	
	struct sockaddr *client_addr = malloc(sizeof(struct sockaddr)); //Pointer just to save the IP address of the client connecting
	memset(client_addr, 0, sizeof(*client_addr));

	//open log
	openlog(NULL,0,LOG_USER);
	
	//Socket creation and connection with client:
	ret = socket_creation(&sockfd, client_addr);	

	if(argc>1){
		if(strcmp(argv[1], "-d") == GOOD_VAL){
			//Start as daemon
			pid_t pid = fork();
			if(pid == 0){
				//This is the child
			}
			else if(pid == -1){
				return -1;
			}
			else{
				exit(EXIT_SUCCESS);
			}
			
			//Create new session and new process group for this daemon to be a leader
			if(setsid() == -1){
				return -1;
			}
			
			//Set working directory to root
			if(chdir("/") == -1){
				return -1;
			}
			
			//Close all open files
			/*for(int i=0; i< sysconf(_SC_OPEN_MAX); i++){
				close(i);
			}*/
			
			//Redirect fd's 0,1,2 to /dev/null
			open("/dev/null", O_RDWR); //stdin
			dup(0);	//stdout
			dup(0); //stderror			
		}
	}
	//Loop until SIGINT or SIGTERM
	while((aesdsock_close_gracefully == false)&&(ret!=ERR_VAL)){ //As long as SIGINT or SIGTERM are not set
		if(ret == GOOD_VAL){
			ret = socket_connection(sockfd, &client_sock, client_addr);
		}
		if(ret!=ERR_VAL){ //If connection is stablished properly
			int outfile = open("/var/tmp/aesdsocketdata", O_CREAT | O_RDWR | O_APPEND, 0777);
			//int outfile = open("/home/david/Documents/_David/_Coursera/aesdtest", O_CREAT | O_RDWR | O_APPEND, 0777);
			if (outfile == -1){
				perror("Error opening aesdsocketdata file");
				ret = ERR_VAL;
				break;
			}
			//char recv_string[RECV_BUFFER];
			char *string_buffer = malloc(RECV_BUFFER);
			memset(string_buffer,'\0',RECV_BUFFER); //Add null everywhere
			size_t extra_size = 0;
			size_t current_size = RECV_BUFFER;
			
			if(outfile != ERR_VAL){ //Create or open aesdsocketdata file
				int bytes_recv = 0;
				int total_bytes_recv = 0;
				ssize_t written_bytes = 0;
					while((bytes_recv = recv(client_sock, string_buffer + total_bytes_recv, RECV_BUFFER, 0)) > 0){ //keep reading while there are bytes received
						total_bytes_recv += bytes_recv;
						                    
						if(strchr(string_buffer, '\n')){ //If there is already a packet end
							string_buffer[total_bytes_recv] = '\0'; //NULL terminate string
							written_bytes = write(outfile, string_buffer, total_bytes_recv); //appends packet
							if(written_bytes<0){
								perror("Error writing to file");
							}
							fsync(outfile);
							extra_size = 0;					
							memset(string_buffer,'\0',RECV_BUFFER); //Add null everywhere
							//Return full content of /var/tmp/aesdsocketdata to client
							return_data_to_client(outfile, client_sock);
							total_bytes_recv = 0;
						}
						else if(total_bytes_recv >= current_size){
							extra_size = RECV_BUFFER;
							char *expanded_buff = realloc(string_buffer, current_size + extra_size); //Expand buffer by the necessary size
							current_size = current_size + extra_size;
							if(expanded_buff == NULL){
								perror("Reallocation failed for recv");
								free(expanded_buff);
								free(string_buffer);
								close(outfile);
								close(client_sock);
								ret = ERR_VAL;
								break;
							}

							string_buffer = expanded_buff; //So now the string buffer is the new expanded_buff
						}
						else{
							extra_size = 0;
						}				
					}
			}
			free(string_buffer);
			close(outfile); //Close file handler
			syslog(LOG_INFO, "Closed connection from %s", client_addr->sa_data);
		}
		else{
			ret = ERR_VAL;
			break;
		}
		close(client_sock);
	}
	remove("/var/tmp/aesdsocketdata");
	close(client_sock);
	close(sockfd);
	
	return ret;
}
