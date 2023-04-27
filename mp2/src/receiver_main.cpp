/*
** creat by kent chen
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <deque>
#include <iostream>

#define PORT "2055"  // the port users will be connecting to
#define BUFFER_SIZE 16384

using namespace std;

struct sockaddr_in server_addr, client_addr, si_other;
int threadno = 0, s, slen;
pthread_t threads [20];

struct thread_packet{
    int order;
    int local_fd;
    char content [BUFFER_SIZE];
    socklen_t addlen;
    sockaddr_in clientaddr;
};

struct Packet {
    int seq;
    // uint32_t length;
    char data[BUFFER_SIZE];
};

void diep(char *s) {
    perror(s);
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    Packet window;
    slen = sizeof (si_other);
    int recvdata, cur_ack, data_seq=0;
    int msgnum = 0;
    char buf [BUFFER_SIZE];
    deque<string> data;
    deque <pair<int, const char*>> data_buf;
    socklen_t addrlen = sizeof(client_addr);
    FILE *fp;
    printf("%s\n", destinationFile);
    fp = fopen(destinationFile, "w");
    if(fp==NULL){
        printf("the file cannot be opened\n");
    }

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(myUDPport);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n-------\n");
    if (bind(s, (struct sockaddr*) &server_addr, sizeof (server_addr)) == -1)
        diep("bind");

	/* Now receive data and send acknowledgements */    
    while(1){
        if(recvdata = recvfrom(s,&window, sizeof(Packet),0, (sockaddr*) &client_addr, &addrlen)>0){
            strcpy(buf,window.data);

            /* if receive end message, close the socket and store the file*/
            if(strcmp(buf, "endend") == 0){
                printf("enter end\n");
                string back_ack = "receive_end";
                const char* back_msg = back_ack.c_str();
                sendto(s, back_msg, sizeof(back_msg),0,(struct sockaddr *) &client_addr, sizeof(client_addr));
                std::cout<<"receive_end "<<back_msg<<endl;
                break;
            }

            // get the packet content and ack number
            cur_ack = window.seq;

            //if the ack number is inorder, store the packet content in file
            if(cur_ack == msgnum){
                fflush(fp);
                if(msgnum == data_seq){
                    fputs(buf, fp);
                }
                //send the next ack number to sender
                string back_ack = to_string(msgnum);
                const char* back_msg = back_ack.c_str();
                sendto(s, back_msg, sizeof(back_msg),0,(struct sockaddr *) &client_addr, sizeof(client_addr));
                std::cout<<"send back ack_num is "<<back_msg<<endl;
                msgnum++;
                data_seq++;
                while(!data_buf.empty()){
                    const char* cur_data = data[0].c_str();
                    fflush(fp);
                    if(msgnum == data_seq){
                        fputs(cur_data, fp);
                    }
                    data_buf.pop_front();
                    data.pop_front();
                    string back_ack = to_string(msgnum);
                    const char* back_msg = back_ack.c_str();
                    sendto(s, back_msg, sizeof(back_msg),0,(struct sockaddr *) &client_addr, sizeof(client_addr));
                    std::cout<<"send back ack_num is "<<back_msg<<endl;
                    msgnum++;
                    data_seq++;
                }
            }

            else if(cur_ack>msgnum){
                //resend the cumulative ack number to sender
                string back_ack = to_string(msgnum-1);
                const char* back_msg = back_ack.c_str();
                sendto(s, back_msg, sizeof(back_msg),0,(struct sockaddr *) &client_addr, sizeof(client_addr));

                //store data in deque buf
                pair<int, const char*> check = {window.seq, buf};
                data_buf.push_back(check);
                string str = string(buf);
                data.push_back(str);
            }
            memset(buf, 0, sizeof(buf));
        }
    }

    fclose(fp);
    close(s);
	printf("%s received.", destinationFile);
    return;
}


void *store_in_file(void* r){
    const char *ack = "ack0";
    struct thread_packet *args = (struct thread_packet *)args ;
    printf("receive first pakcets %", args->content);
    sendto(args->local_fd, (const char *)ack, strlen(ack),0,
            (struct sockaddr *) &args->clientaddr, sizeof(args->clientaddr));
}
/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}


