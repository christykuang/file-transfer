//1. open a UDP socket and listen at the specified port number
//2. receive a message from the  client
   // a. if the message is "ftp",  reply with  a  message "yes"  to the client
   //b.else,reply with  a  message "no" to the  client


#include  <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define  buffersize 1200

typedef struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
}Packet;



void receive_file(int recsockfd, struct sockaddr_in clientAddrInfo){
    char buffer[buffersize];
    socklen_t clientAddrInfo_size = sizeof(clientAddrInfo);
    //Packet PacketCreated;
    //PacketCreated.total_frag = 5;
    //PacketCreated.frag_no = 1;
    FILE *fileStoreData;
    srand((unsigned)time(NULL));
    while(1){
        //receive message from deliver
        int count= 0;
        Packet PacketCreated;
        char totalFrag[100];
        char frag_no[100];
        char packetSize[100];
        char filenameReceiced[100];
        
        
        int numBytesReceived = recvfrom(recsockfd, buffer,  buffersize, 0,  
                                (struct sockaddr *)&clientAddrInfo, &clientAddrInfo_size);
        int b = rand()%20;
        printf("rand number: %d\n", b);
        if(b==0){
            printf("packet drops occur.\n");
        }
        else{
            if(numBytesReceived <= 0){
                printf("receive packet message failed!\n");
                close(recsockfd);
                exit(1);
            }
            //create packet
            int divideLine = 0;
            int DataStartIndex = 0;
            // “3:2:10:foobar.txt:lo World!\n”
            // read the value from message
            for(int i = 0; i < buffersize; i++){
                if(buffer[i] == ':'){
                    switch(count){
                        case 0:
                        {   totalFrag[i] = '\0';
                            divideLine = i + 1;//next qunatity start point;
                            count += 1;
                            continue;
                        }
                        case 1:
                        {
                            frag_no[i-divideLine] = '\0';
                            divideLine = i + 1;//next qunatity start point;
                            count += 1;
                            continue;
                        }
                        case 2:
                        {
                            packetSize[i-divideLine] = '\0';
                            divideLine = i + 1;//next qunatity start point;
                            count += 1;
                            continue;
                        }
                        case 3:
                        {
                            filenameReceiced[i-divideLine] = '\0';
                            divideLine = i + 1;//next qunatity start point;
                            count += 1;
                            continue;
                        }
                    }
                }
                else{
                    switch(count){
                        case 0:
                            totalFrag[i] = buffer[i];
                            continue;
                        case 1:
                            frag_no[i-divideLine] = buffer[i];
                            //printf("frag_no[%d]: %c\n", i-divideLine, buffer[i]); 
                            continue;
                        case 2:
                            packetSize[i-divideLine] = buffer[i];
                            continue;
                        case 3:
                            filenameReceiced[i-divideLine] = buffer[i];
                            continue;
                        case 4:
                            DataStartIndex = i;
                            break;
                    }
                    break;
                }
            }

            //store into packet;
            PacketCreated.total_frag = atoi(totalFrag);
            PacketCreated.frag_no = atoi(frag_no);
            //printf("frag_no: %d\n", PacketCreated.frag_no);
            PacketCreated.size = atoi(packetSize);
            PacketCreated.filename = (char *)malloc(strlen(filenameReceiced) + 1);
            strcpy(PacketCreated.filename, filenameReceiced);
            //printf("no.%d in %d receive packet message successfully\n", PacketCreated.frag_no, PacketCreated.total_frag);
            //store data
            for(int i = 0; i < PacketCreated.size; i++){
                PacketCreated.filedata[i] = buffer[DataStartIndex+i];
            }
            //acknowledgement of receiving message;
            int ACKbyte = sendto(recsockfd,"yes",strlen("yes"),0, (struct sockaddr*)&clientAddrInfo, clientAddrInfo_size);
            if(ACKbyte<=0){
                printf("fail to send ACK\n");
                close(recsockfd);
                exit(1);
            }
            //create file 
            if(PacketCreated.frag_no == 1){
                //create a file after the first packet received
                fileStoreData = fopen(PacketCreated.filename, "w");
                if (!fileStoreData){
                    printf("Failed to create file");
                    exit(1);
                }
                fwrite(PacketCreated.filedata, 1, PacketCreated.size, fileStoreData); 
            }
            else{
                //just write file;
                fwrite(PacketCreated.filedata, 1, PacketCreated.size, fileStoreData); 
            }

            if(PacketCreated.frag_no == PacketCreated.total_frag){
                break;
            }

            
        }
        
    }
    fclose(fileStoreData);
    return;

}

int main(int argc, char *argv[]){

    //some code come from https://www.youtube.com/watch?v=IUyaV4haBUE

    unsigned char buffer[buffersize];
    int deliverSocket;
    struct sockaddr_in clientAddrInfo;
    struct sockaddr_in serverAddrInfo={0};

    //create socket
    int recsockfd = socket(AF_INET,SOCK_DGRAM, 0);
    if(recsockfd == -1){
        printf("fail to create socket");
        exit(1);
    }

    //listening port number
    int listenPort = atoi(argv[1]);

    serverAddrInfo.sin_family = AF_INET;//ipv4
    serverAddrInfo.sin_port = htons(listenPort); //listen  port
    serverAddrInfo.sin_addr.s_addr = htonl(INADDR_ANY);
    
    socklen_t len =sizeof(struct sockaddr);
    //bind associates a  socket with an  address
    int bindVal = bind (recsockfd,(const struct  sockaddr*)&serverAddrInfo, len);
    //check if  bind  success  of  not 
    if(bindVal == -1){ //!=0 bind fails
        printf("fail to  bind");
        close(recsockfd);
        exit(1);
    }
    
    
    //receive  message from client
    socklen_t clientAddrInfo_size = sizeof(clientAddrInfo);
    int returnByte = recvfrom(recsockfd,buffer,buffersize,0,(struct sockaddr*) &clientAddrInfo,&clientAddrInfo_size);
    printf("%s\n",buffer);
    if(returnByte <=0){
        printf("fail to  receive\n");
        close(recsockfd);
        exit(1);
    }
    
    if(strncmp(buffer,"ftp",3)==0){
        //reply "yes"  to the client
        int byte = sendto(recsockfd,"yes",strlen("yes"),0, (struct sockaddr*) &clientAddrInfo, clientAddrInfo_size);
        
    }
    else{
        //reply "no"
        int byte = sendto(recsockfd,"no",strlen("no"),0,(struct sockaddr*) &clientAddrInfo, clientAddrInfo_size);
    }
    ////////////////////////section 2///////////////////////////////////////
    //upon receiving the first packet in a sequence (frag no = 1),the program should create a struct packet containing the received data, read the 
    //file name from it and create a corresponding file stream on the local file system(received file must have the same name  as the send file)
    //file data read from the packets should then be written to this file stream
    //when last packet is received, the file stream should be closed
    
    //receive file 
    receive_file(recsockfd, clientAddrInfo);

    printf("transfer done\n");
    close(recsockfd);
    return 0;




}
        perror("Failed to receive from clients\n"); 
                    return -1;
                }
                else{
                    //process_recv_data(clietn_socket, recv_msg);
                    deserialize(receivebuffer, &recMessage);
                    //printf("type:%d, size: %d, source: %s, data: %s\n", recMessage.type, recMessage.size, recMessage.source, recMessage.data);
                }
                if(recMessage.type == LOGIN){
                    int add = login(recMessage, new_socket_fd, client_addr);  
                    totalClients = totalClients + add;
                }
                //not login message;
                else{
                    printf("first message from clients is not logged in message, so not activate login state\n");
                }
                
            }

            //READING from logined clients
            for(int i = 0; i < MAX_USERS; i++){
                if(FD_ISSET(clientList[i].sockClientfd, &readSet)){
                    char receivebuffer[1200];
                    memset(receivebuffer, 0, 1200*sizeof(char));
                    int recvByte = recv(clientList[i].sockClientfd, receivebuffer, 
                                    sizeof(receivebuffer), 0);
                    //printf("recevBuffer: %s\n", receivebuffer);
                    if (recvByte == 0) {
                        printf("Client Disconnected\n"); 
                        Exit(i);
                    }
                    else if(recvByte == -1){
                        perror("Failed to receive from clients\n"); 
                        return -1;
                    }
                    else{
                        struct message recMessage;
                        memset(recMessage.data, 0, sizeof(recMessage.data));
                        deserialize(receivebuffer, &recMessage);
                        //printf("type:%d, size: %d, source: %s, data: %s\n", recMessage.type, recMessage.size, recMessage.source, recMessage.data);
                        if(recMessage.type == LOGIN){
                            //assume same logged in cilentID join again;
                            //only one case: already logined in, send LO_NAK
                            char sendbuffer[1000];
                            //If already login
                            if(clientList[i].loggedIn == 1){
                                //send LO_NAK
                                char data[] = "User Already Loged in";
                                int sendbyte = serialize(LO_NAK, strlen(data), recMessage.source, data, sendbuffer);
                                //Send message
                                send(clientList[i].sockClientfd, sendbuffer, sendbyte, 0);
                                printf("LO_NAK: Successfully send the message of LO_NAK\n");
                            }
                        }
                        else if(recMessage.type == EXIT){
                            //initialize clientList[i]
                            Exit(i);
                        }
                        else if(recMessage.type == JOIN){
                            Join(recMessage, i);
                        }
                        else if(recMessage.type == LEAVE_SESS){
                            //printf("In the Leave_sess\n");
                            LeaveSession(i);
                        }
                        else if(recMessage.type == NEW_SESS){
                            createNewSession(i, recMessage);
                            
                        }
                        else if(recMessage.type == MESSAGE){
                            multiForwarding(i, recMessage);
                        }
                        else if(recMessage.type == QUERY){
                            query(i, recMessage);
                            //printf("recMessage: %s\n", recMessage.data);
                        }
                    }
                }
            }
        }



    }
    return 0;
}

//source of recmessage is client_id
//compared to existed client id in the clientList
//if already logined in, then send LO_NAK
//if not, send LO_ACK
    //add new client: update clientid, password, ip, port, sockClientfd, loggedInstate)
int login(struct message recMessage, int new_socket_fd, struct sockaddr_in client_addr){

    int clientIndexInList;
    for(int i = 0; i < MAX_USERS; i++){
        //alreay logined 
        if(strcmp(clientList[i].clientsId, recMessage.source)==0){
            if(clientList[i].loggedIn == 1){
                //send LO_NAK
                char sendbuffer[1000];
                memset(sendbuffer, 0, sizeof(sendbuffer));
                char data[] = "User Already Loged in";
                int sendbyte = serialize(LO_NAK, strlen(data), recMessage.source, data, sendbuffer);
                //Send message
                send(new_socket_fd, sendbuffer, sendbyte, 0);
                printf("LO_NAK: Successfully send the message of LO_NAK\n");
                return 0;
            }
        }
        if(strcmp(clientList[i].clientsId, "")==0){
            //find first empty element in clientList
            clientIndexInList = i;
        }
    }

    //if not, send LO_ACK
    char sendbuffer[1000];
    memset(sendbuffer, 0, sizeof(sendbuffer));
    int sendbyte = serialize(LO_ACK, 1, recMessage.source, "0", sendbuffer);
    //Send message
    send(new_socket_fd, sendbuffer, sendbyte, 0);
    printf("LO_ACK: Successfully send the message of LO_ACK\n");
    //add new client: update clientid, password, ip, port, sockClientfd, loggedInstate)
    strcpy(clientList[clientIndexInList].clientsId, recMessage.source);
    strcpy(clientList[clientIndexInList].password, recMessage.data);
        //get the Client's IP and Port
        //convert to host byte
    int clientPort = ntohs(client_addr.sin_port);
    char clientIp[100]="0";
    inet_ntop(AF_INET, &(client_addr.sin_addr), clientIp, (socklen_t)100);
    printf("LOGIN: New Client with Port %d, IP %s\n", clientPort, clientIp);
    strcpy(clientList[clientIndexInList].ip, clientIp);
    clientList[clientIndexInList].port = clientPort;
    clientList[clientIndexInList].sockClientfd = new_socket_fd;
    clientList[clientIndexInList].loggedIn = 1;
    return 1;
}

void Exit(int index){
    //client close fd
    //initialize client[i];
    clientList[index].port = 0;
    close(clientList[index].sockClientfd);
    clientList[index].sockClientfd = -1;
    clientList[index].loggedIn = 0;
    memset(clientList[index].clientsId, 0, 100 * sizeof(char));
    memset(clientList[index].password, 0, 100 * sizeof(char));
    memset(clientList[index].sessionId, 0, 100 * sizeof(char));
    memset(clientList[index].ip, 0, 100 * sizeof(char));
    printf("EXIT: %s exit the server\n", clientList[index].clientsId);
}

//JN_ACK: sessionID exist & client not join any session
        //sessionList with that id, total clients ++, clientlist[i].sessionID update
    //JN_NAK: sessionID not exist or client join a session(same one, other one)
void Join(struct message recMessage, int index){
    //check sessionId existed or not
    bool isSession = false;
    int sessionIDIndex;
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(sessionList[i].sessionId, recMessage.data)==0){
            //session exist
            isSession = true;
            sessionIDIndex = i;
            break;
        }
    }
    if(isSession == false){
        //JN_NAK: sessionId not exist
        //send JN_NAK
        char sendbuffer[1000];
        memset(sendbuffer, 0, sizeof(sendbuffer));
        char data[] = "sessionId not exist\n";
        int sendbyte = serialize(JN_NAK, strlen(data), recMessage.source, data, sendbuffer);
        //Send message
        //printf("JOIN:sendbuffer: %s\n", sendbuffer);
        send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
        printf("JOIN: Successfully send the message of JN_NAK\n");
        return;
    }

    //check if client is joined the session or not
    //JN_NAK:client already join the same session
    if(strcmp(clientList[index].sessionId, recMessage.data)==0){
        //JN_NAK: sessionId not exist
        //send JN_NAK
        char sendbuffer[1000];
        memset(sendbuffer, 0, sizeof(sendbuffer));
        char data[] = "client already join the same session\n";
        int sendbyte = serialize(JN_NAK, strlen(data), recMessage.source, data, sendbuffer);
        //Send message
        //printf("JOIN:sendbuffer: %s\n", sendbuffer);
        send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
        printf("JOIN: Successfully send the message of JN_NAK\n");
        return;
    }
    //JN_NAK:already joined a different session
    else if(strcmp(clientList[index].sessionId, "")!=0){
        //JN_NAK: sessionId not exist
        //send JN_NAK
        char sendbuffer[1000];
        memset(sendbuffer, 0, sizeof(sendbuffer));
        char data[] = "client already join a different session\n";
        int sendbyte = serialize(JN_NAK, strlen(data), recMessage.source, data, sendbuffer);
        //Send message
        send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
        printf("JOIN: Successfully send the message of JN_NAK\n");
        return;
    }
    //JN_ACK: client not join any session & sessionId exist
    //sessionList with that id, total clients ++, clientlist[i].sessionID update
    else
    {
        sessionList[sessionIDIndex].totalClients++;
        //clientlist[i].sessionID update
        strcpy(clientList[index].sessionId, recMessage.data);
        //SEND : JN_ACK:SESSIONID
        char sendbuffer[1000];
        memset(sendbuffer, 0, sizeof(sendbuffer));
        int sendbyte = serialize(JN_ACK, recMessage.size, recMessage.source, recMessage.data, sendbuffer);
        //Send message
        send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
        printf("JOIN: Successfully send the message of JN_ACK\n");
        return;
    }
    
}

void LeaveSession(int index){
    //nothing need to send

    
    //sessionList total clients--
        //edge case: if num of ppl after this is 0, then delete session
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(clientList[index].sessionId, sessionList[i].sessionId)==0){
            sessionList[i].totalClients--;
            //printf("sessionid[%d]: %s totalClients: %d\n", i, sessionList[i].sessionId, sessionList[i].totalClients);
            if(sessionList[i].totalClients==0){
                //if num of ppl after this is 0, then delete session
                memset(sessionList[i].sessionId, 0, 100 * sizeof(char));
            }
            printf("LEAVE: Move %s from session %s\n", clientList[index].clientsId, clientList[index].sessionId);
            //client sessionID initialize to nothing
            memset(clientList[index].sessionId, 0, 100 * sizeof(char));
            return;
        }
    }


}

//send NS_ACK
//find first empty element in sessionList
//add a new sessionid in the sessionList
//update totalclients=1 in that session
//clientList[i].sessionId update
void createNewSession(int index, struct message recMessage){
    //send NS_ACK
    char sendbuffer[1000];
    memset(sendbuffer, 0, sizeof(sendbuffer));
    int sendbyte = serialize(NS_ACK, 1, recMessage.source, "0", sendbuffer);
    //Send message
    send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
    printf("NEW_SESS: Successfully send the message of NS_ACK\n");

    for(int i = 0; i < MAX_USERS; i++){
        if(sessionList[i].totalClients == 0){
            //find first empty element in sessionList
            //add a new sessionid in the sessionList
            strcpy(sessionList[i].sessionId, recMessage.data);
            //update totalclients=1 in that session
            sessionList[i].totalClients = 1;
            //printf("sessionList[%d].sessionId: %s\n", i, recMessage.data);
            //printf("sessionList[%d].totalClients: %d\n", i, sessionList[i].totalClients);
            break;
        }
    }
    //update client sessionId
    //printf("clientList[%d].sessionId: %s\n", index, recMessage.data);
    
    strcpy(clientList[index].sessionId, recMessage.data);
    return;

}

//send message to everyone in that session except the client that sent it
    //we have index i to track client
    //go through all client list and if session id = sentclient session id(but clientID not equalt o sent people)
    //send message to them
void multiForwarding(int index, struct message recMessage){
    for(int i = 0 ; i < MAX_USERS; i++){
        //everyone in that session
        if(strcmp(clientList[i].sessionId, clientList[index].sessionId)==0){
            //except the client that sent it
           if(i != index){
                //send MESSAGE
                char sendbuffer[1000];
                memset(sendbuffer, 0, sizeof(sendbuffer));
                int sendbyte = serialize(MESSAGE, recMessage.size, recMessage.source, recMessage.data, sendbuffer);
                //Send message
                send(clientList[i].sockClientfd, sendbuffer, sendbyte, 0);
                printf("BROADCAST: Successfully send the MESSAGE\n");
           }
        }
    }
    return;
}

void query(int index, struct message recMessage){
    
    char listOfClientsWithSessionId[200] = " ";
    //memset(listOfClientsWithSessionId, 0, sizeof(listOfClientsWithSessionId));

    for(int i = 0; i < MAX_USERS; i++){
        if(clientList[i].loggedIn == 1){
            strcat(listOfClientsWithSessionId, clientList[i].clientsId);
            strcat(listOfClientsWithSessionId, " -> ");
            //not in any session
            if(strcmp(clientList[i].sessionId, "")==0){
                strcat(listOfClientsWithSessionId, "no session");
            }
            else{
                strcat(listOfClientsWithSessionId, clientList[i].sessionId);
            }
            strcat(listOfClientsWithSessionId, " ");
        }
    }
    //send MESSAGE
    char sendbuffer[1000];
    memset(sendbuffer, 0, sizeof(sendbuffer));
    int sendbyte = serialize(QU_ACK, strlen(listOfClientsWithSessionId), recMessage.source, 
                    listOfClientsWithSessionId, sendbuffer);
    //printf("Send List: %s\n", sendbuffer);
    //Send message
    send(clientList[index].sockClientfd, sendbuffer, sendbyte, 0);
    printf("QUERY: Successfully send the list of users and session\n");
    return;
}