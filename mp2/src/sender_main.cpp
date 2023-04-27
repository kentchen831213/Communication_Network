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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <ctime>
#include <deque>
#include <vector>
#include <chrono>
#include <thread>
#define PORT "2055"  // the port users will be connecting to
#define BUFFER_SIZE 1024
#define FILE_BUFFER 4094
#define TIMEOUT 1000
#define CWND 8

using namespace std;

struct sockaddr_in si_other;
struct timeval tv,timeout, tt;
int s, slen;
int cnwd=1;

void diep(char *s) {
    perror(s);
    exit(1);
}

struct Packet {
    int seq;
    char data[BUFFER_SIZE];
};

void gbn(int sock_fd, FILE * fp, struct sockaddr_in servaddr, int window_size, unsigned long long int bytesToTransfer){
    Packet windows[1024];
    deque <pair<int, uint64_t>> timer;
    int seq, base =0, nextseqnum = 0;
    char receive_buf[30];
    int idx, n;
    socklen_t addrlen;
    bool eof,flag = false;
    time_t timer_end;
    // Send initial packets
    while (!feof(fp)){
        //rdt_send(data)
        while (nextseqnum < base+window_size and !feof(fp)){
            idx = nextseqnum - base;
            fread(&windows[idx].data, sizeof(windows[idx].data), 1, fp);
            windows[idx].seq = nextseqnum;
            //send packet to receiver
            sendto(sock_fd, &windows[idx], sizeof(Packet),0, (struct sockaddr *) &servaddr, sizeof(servaddr));
            //start timer of each packet, put in deque inorder
            gettimeofday(&timeout, NULL);
            uint64_t ms = timeout.tv_sec*1000+timeout.tv_usec/1000;
            std::cout<<"\nms is "<<ms<<endl;
            ms /= 1000;
            timer.push_back({nextseqnum, ms});
            nextseqnum++;
        }

        bool add_cwnd = true;

        //receiving back ack and remove the corresponding packets number from timer's queue
        while(!timer.empty()){
            gettimeofday(&tt, NULL);
            uint64_t cur_time = tt.tv_sec*1000+tt.tv_usec/1000;
            uint64_t start_time = timer.begin()->second;
            // std::cout<<"enter11"<<endl;
            // if time out, resend remaining packets
            if(cur_time-start_time>TIMEOUT){
                std::cout<<"cur_time"<<cur_time<<endl;
                std::cout<<"start_time"<<start_time<<endl;
                int start = window_size - (nextseqnum - base);
                for(int i = base; i<nextseqnum;i++){
                    sendto(sock_fd, &windows[start], sizeof(Packet),0, (struct sockaddr *) &servaddr, sizeof(servaddr));
                    // restart timer
                    timer.pop_front();
                    pair<int, time_t> curtimer = {i,time(NULL)};
                    timer.push_back(curtimer);
                    start++;
                }
                add_cwnd = false;
                printf("resend packets");
            }

            //if receive ack, remove the corressponding timer from the queue
            n = recvfrom(sock_fd, receive_buf, sizeof(receive_buf), 0,(struct sockaddr *) &servaddr, &addrlen);
            
            if(!timer.empty() && atoi(receive_buf)>=timer.begin()->first){
                std::cout<<"receive ack is "<<atoi(receive_buf)<<endl;
                timer.pop_front();
                base++;
            }

            //if receive ack smaller than current seder ack, resend the remaining packets(packets loss)
            else if(atoi(receive_buf)< timer.begin()->first){
                int start = window_size - (nextseqnum - base);

                sendto(sock_fd, &windows[start], sizeof(Packet),0, (struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("resend lost packets");
                // restart timer
                timer.pop_front();
                pair<int, time_t> curtimer = {base,time(NULL)};
                timer.push_front(curtimer);
                
            }
            memset(receive_buf,0, sizeof(receive_buf));
            // sleep(1);
        }
        /*clean the window buffer*/
        for(int i = 0; i<window_size;i++)
            memset(windows[i].data,0, sizeof(windows[i].data));
        
        if(add_cwnd){
            window_size *= 2;
        }
        else{
            if(window_size==16){
                window_size = 16;
            }
            else{
                window_size /=2;
            }
        }
        std::cout<<"window size is: "<<window_size<<endl;
    }

    /*send end sign*/
    Packet window;
    strcpy(window.data,"endend");
    while(1){
        sendto(sock_fd, &window, sizeof(Packet),0, (struct sockaddr *) &servaddr, sizeof(servaddr));
        n = recvfrom(sock_fd, receive_buf, sizeof(receive_buf), 0,(struct sockaddr *) &servaddr, &addrlen);
        if(strcmp(receive_buf,"receive_end")){
            std::cout<<"receive end ack"<<endl;
            break;
        }
    }
    memset(receive_buf,0, sizeof(receive_buf));
    return;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // setting timout
    // tv.tv_sec = TIMEOUT;
    tv.tv_usec = 20;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

	/* Send data and receive acknowledgements on s*/
    gbn(s, fp, si_other, CWND, bytesToTransfer);

    printf("Closing the socket\n");
    close(s);
    return;

}


int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;
    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);
    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
    return (EXIT_SUCCESS);
}