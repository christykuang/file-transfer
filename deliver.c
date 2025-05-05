#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#define EXIT_FAILTURE -1
#define a 0.125
struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main(int argc, char *argv[]){
    //some code comes from https://blog.csdn.net/TECH_PRO/article/details/71106469 and Beej's Guide

    char input_message[100];
    char prefix[100];
    char file_name[100];
    clock_t startTime;
    clock_t endTime;
    struct timeval timeout;
    //long RTT;
    double estimatedRTT;
    FILE *file;
    //section 1
    printf("Input a message in this form: ftp <file name>\n");
    //ask user to input name of file
    //fgets(input_message, 100, stdin);
    scanf("%[^\n]",input_message);
    //extreact the file name from the input message
    for(int i = 4; i <= strlen(input_message); i++){//strlen()do not count null character
        file_name[i-4] = input_message[i];
    }

    struct sockaddr_in tSocketservaddr = {0};
    int sockDeliverfd = socket(AF_INET, SOCK_DGRAM, 0);//ipv4 and udp
    //socket==-1 means failed
    if ( sockDeliverfd < 0 ) { 
        perror("failed to create socket"); 
        return -1; 
    } 
    
    int port = atoi(argv[2]);//second command-line argument into an integer
    int IPn;
    // Filling server information 
    tSocketservaddr.sin_family = AF_INET;//ipv4
    tSocketservaddr.sin_port = htons(port); //port of the recipient
    IPn = inet_aton(argv[1], &tSocketservaddr.sin_addr);//ip address of the recipient
    if(IPn == 0){
        printf("server IP address is not valid or existing");
        exit(EXIT_FAILTURE);
    }

    //check existence of the file
    
    //printf("%s\n", file_name);
    if(access(file_name, F_OK) == -1){
        printf("file doesn't exist!\n");
	    return 1;
    }
    //file exists
    
    
    else{
        //send ftp to server
        startTime = clock();
        printf("file exist!\n");
        int numOfBytesSend = sendto(sockDeliverfd, "ftp\n", strlen("ftp\n"), 
                                0, (const struct sockaddr *) &tSocketservaddr, 
                                sizeof(tSocketservaddr));
        if(numOfBytesSend <= 0){
            printf("sento error");
            close(sockDeliverfd);
            exit(EXIT_FAILTURE);
        }

    }
    //fclose(file);

    //receiving message from server: yes
    socklen_t tSocketservaddr_size = sizeof(tSocketservaddr);
    char buffer[10];
    int numOfBytesReceived = recvfrom(sockDeliverfd, buffer, sizeof(buffer), 0, 
    (struct sockaddr *) &tSocketservaddr, &tSocketservaddr_size);
    //calculate round trip time;
    endTime = clock();
    //timeout.tv_usec = (end.tv_usec - start.tv_usec)*1e6*4;
    //timeout.tv_sec = end.tv_sec - start.tv_sec;
    //RTT = (end.tv_usec - start.tv_usec)/1000;
    estimatedRTT = ((double)(endTime-startTime)/CLOCKS_PER_SEC);
    timeout.tv_sec = 0;
    timeout.tv_usec = estimatedRTT * 1e6 * 5;
    setsockopt(sockDeliverfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    //printf("start time: %ld . %ld\n", start.tv_sec, start.tv_usec);
    //printf("end time: %ld . %ld\n", end.tv_sec, end.tv_usec);
    printf("Round-trip time is %f seconds\n", estimatedRTT);
    printf("timeout is %ld. %ld microseconds\n", timeout.tv_sec, timeout.tv_usec);
    if (numOfBytesReceived <= 0){
        printf("recvfrom error!\n");
        close(sockDeliverfd);
        exit(EXIT_FAILTURE);
    }

    if (strncmp(buffer, "yes", 3) == 0){
        printf("A file transfer can start\n");
    }
    else{
        printf("A file transfer can't start\n");
        close(sockDeliverfd);
        exit(EXIT_FAILTURE);
    }
    ////////////////////////////////////////////
    ////////////////Section 2///////////////////
    ////////////////////////////////////////////
    
    //open the binary file
    file = fopen(file_name, "rb");
    printf("file_name: %s\n", file_name);
    if (file == NULL) {
        printf("fopen failed\n");
        exit(EXIT_FAILTURE);
    }
    else{
        printf("fopen is successful\n");
    }
    //get size of file
    fseek(file, 0, SEEK_END);//move pointer to the end of file
    long sizeOfFile = ftell(file);
    //fseek(file, 0, SEEK_SET);
    rewind(file);
    
    printf("sizeOfFile: %ld\n", sizeOfFile);

    // determine number of fragments of file
    int num_frag = (sizeOfFile/1000) + (sizeOfFile % 1000 == 0 ? 0 : 1);
    printf("num_frag: %d\n", num_frag);
    //determine last packet size of data
    //int lastSize =  sizeOfFile % 1000;

    //allocate memory for buffer and can read in all bytes
    char *packetBuffer;
    packetBuffer = (char *)malloc(sizeof(char) * sizeOfFile);
    fread(packetBuffer, sizeof(char), sizeOfFile, file);
    fclose(file);
    printf("buffer loaded successfully\n");

    //calculate the total number of packets needed and put into an array
    int Totalpackets = num_frag;
    struct packet* parray = malloc(Totalpackets*sizeof(struct packet));
    //create packet array
    for(int i = 0; i < Totalpackets; i++){
        struct packet singlePacket;
        singlePacket.total_frag = Totalpackets;
        singlePacket.frag_no = i + 1;
        if(i == Totalpackets - 1){
            singlePacket.size = sizeOfFile - (Totalpackets - 1)*1000;
        }
        else{
            singlePacket.size = 1000;
        }
        singlePacket.filename = file_name;
        for(int j = 0; j < singlePacket.size; j++){
            singlePacket.filedata[j] = packetBuffer[i*1000+j];
        }
        parray[i] = singlePacket;

    }
    printf("create packet array\n");

    
    printf("Totalpackets: %d\n", Totalpackets);
    //sending packet;
    for(int i = 0; i < Totalpackets; i++){
        //printf("into sending successfully\n");
        char sendingMessageBuffer[1200];
        int totalSize_message = sprintf(sendingMessageBuffer, "\n%d:%d:%d:%s:", 
                                parray[i].total_frag, parray[i].frag_no, parray[i].size, parray[i].filename);
        memcpy(&sendingMessageBuffer[totalSize_message], 
                parray[i].filedata, parray[i].size);

        //start of round-trip time
        startTime = clock();

        //sending an packet to the server
        int num_BytesSend = sendto(sockDeliverfd, sendingMessageBuffer, totalSize_message+parray[i].size, 
                                0, (const struct sockaddr *) &tSocketservaddr, 
                                sizeof(tSocketservaddr));
        printf("sending packet successfully\n");
        //sento failed                         
        /*if(num_BytesSend <= 0){
            printf("file sento error");
            close(sockDeliverfd);
            exit(EXIT_FAILTURE);
        }*/

        
        while(1){
            //receive yes from server
            printf("into receiveing ack of No.%d packet\n", i+1);
            char fileBuf[10];
            int num_recBytes = recvfrom(sockDeliverfd, fileBuf, sizeof(fileBuf), 0, 
                    (struct sockaddr *) &tSocketservaddr, &tSocketservaddr_size);
            //timeout option is triggered
            if(num_recBytes < 0){
                printf("No.%d packet needs to be retransmitter\n", i+1);
                //reset the timer, time the start again when retransmitted
                //measure how many clicks
                startTime = clock();
                int num_BytesSend = sendto(sockDeliverfd, sendingMessageBuffer, totalSize_message+parray[i].size, 
                                0, (const struct sockaddr *) &tSocketservaddr, 
                                sizeof(tSocketservaddr));
                
                //sento failed 
                if(num_BytesSend <= 0){
                    printf("file retransmitted sento error");
                    close(sockDeliverfd);
                    exit(EXIT_FAILTURE);
                }
            }
            //successfully received ACK acknowledgement
            else if(strncmp(fileBuf, "yes", 3) == 0){
                endTime = clock();
                //exponentially wighted moving average(EWMA)
                estimatedRTT = ((1 - a) * estimatedRTT) + (a * ((double)(endTime - startTime) / CLOCKS_PER_SEC));
                timeout.tv_sec = 0;
                timeout.tv_usec = estimatedRTT * 1e6 * 5;
                setsockopt(sockDeliverfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                printf("estimatedRTT is %f seconds\n", estimatedRTT);
                printf("timeout is %ld. %ld microseconds\n", timeout.tv_sec, timeout.tv_usec);

                printf("No.%d packet transmitted successfully\n", i+1);
                break;
            }

        }

    }

    close(sockDeliverfd);
    return 0;
}
