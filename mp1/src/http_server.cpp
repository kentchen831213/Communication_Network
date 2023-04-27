/*
** creat by kent chen
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "2055"  // the port users will be connecting to
#define BUFFER_SIZE 4096
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_file_content(int sock_fd, FILE * file){

	char buffer[BUFFER_SIZE];
	int bytes_read;

	// read file content 
	while (!feof(file)){
		if((bytes_read = fread(&buffer,1, BUFFER_SIZE,file))>0)
			send(sock_fd,buffer,bytes_read,0);
		else
			break;
	}

}

void send_bad_request(int sock_fd){

	char buffer[BUFFER_SIZE];
	sprintf(buffer, "HTTP/1.0 404 File Not Found\r\n");
	send(sock_fd,buffer, BUFFER_SIZE,0);
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	const char* port = "80";
	char buffer[BUFFER_SIZE];
	char file_buffer[BUFFER_SIZE];
	FILE *file;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if (argc == 2) {
  		port = argv[1];
 		}
 	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
 		 fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  		return 1;
 	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			memset(buffer, 0, sizeof(buffer));
			int len = recv(new_fd, buffer, sizeof(buffer),0);
			char* split = strtok(buffer,"\n");
			char* http_header = strtok(split,"/");
			http_header = strtok(NULL, " HTTP");

			if(file = fopen(http_header,"rb")){

				send_file_content(new_fd, file);

				fclose(file);
			}
			else{
				send_bad_request(new_fd);
			}
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}