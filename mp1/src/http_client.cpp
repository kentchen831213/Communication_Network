/*
** creat by kent chen
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

#include <arpa/inet.h>
#include <iostream>
using namespace std;
#define PORT "2001" // the port client will be connecting to 

#define BUFFER_SIZE 4096 // max number of bytes we can get at once 

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
	char buffer[BUFFER_SIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char filename, link;
	FILE *fp;
	size_t numwritten;
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	string str = string(argv[1]);
	int idx_begin = str.find("//");
	int idx_before_port = str.find(":",5)+1;
    int idx_end_port = str.find("/", idx_before_port);
    string ip = str.substr(idx_begin+2, idx_before_port-idx_begin-3);
    string port = str.substr(idx_before_port, idx_end_port-idx_before_port);
    string file_name = str.substr(idx_end_port);
    string request = "GET " + file_name + " HTTP/1.1\r\n\r\n";
	cout<<"url"<<ip<<"\n"<<endl;
		//<<"port"<<port;
	strcpy(buffer,request.c_str());

	if ((rv = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
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
			close(sockfd);
			perror("client: connect");
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

	send(sockfd,buffer,sizeof(buffer),0);

	fp = fopen("output.txt", "w");
	if (fp == NULL){
		printf("the file cannot be opened\n");
	}
	

	while ((numwritten = recv(sockfd, buffer, sizeof((buffer)),0))){
		// numwritten = recv((sock_fd, buf, MAXDATASIZE,0));
		fwrite(&buffer, 1, numwritten, fp);
	}


	fclose(fp);
	close(sockfd);

	return 0;
}
