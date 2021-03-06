
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <signal.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>  
#include <pthread.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "message.h"

#define MAX_MEMBERS 5        //total clients that can be handled
// #define MAX_GROUPS_NUM 10           //max number of groups that can be created in the server
// #define MAX_GROUP_SIZE 100          //number of members in a group
#define MAILBOX_SIZE 1000               //size of message queue
#define POSTMAN_COUNT 3                 //Postman reads messages from the message queue and delivers it to it's respective user

// resource handlers
int members_socks[MAX_MEMBERS] = {0};
char members[MAX_MEMBERS][20] = {0};
// char groupNames[MAX_GROUPS_NUM][20];
// int connectionCount[MAX_GROUPS_NUM];
int memberCount = 0;
int socket_fd = 0;  
// int connection_fd = 0;




pthread_mutex_t memberRegistrationMutex;
pthread_t clientHandlerThreads[MAX_MEMBERS];

// message recvMessage;


void terminate(int num);
void *clientHandler(void *sock_fd);
void printMembers(void);


message not_found;



int main(int argc, char **argv){ 

    

    if(argc == 1){
        printf("Please supply a port number.\nExiting....\n");
        exit(0);
    }
    char *portno = argv[1];
    int sock_desc, conn_desc, len; 
    struct sockaddr_in servaddr, cli; 
    pthread_t read_thread, write_thread;
  
    // socket create and verification 
    socket_fd = socket(AF_INET, SOCK_STREAM, 0); 

    if (socket_fd == -1) { 
        printf("Unable to create socket !!\nExitting....\n"); 
        exit(0); 
    } 
    
    printf("Socket successfully created\n"); 
    signal(SIGINT, terminate);

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(atoi(portno)); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("Socket binding Failed\n"); 
        exit(0); 
    }    
    printf("Socket bound Succesfuly\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(socket_fd, 5)) != 0) { 
        printf("Listen failed\n Exiting.....\n"); 
        exit(0); 
    }
    printf("Server listening on Port %i\n\n",atoi(portno)); 


    len = sizeof(cli); 
    // Accept the data packet from client and verification 

    strcpy(not_found.sender, "server");
    

    while(1){
        int *connection_fd = malloc(sizeof(int));
        *connection_fd = accept(socket_fd, (struct sockaddr*)&cli, &len); 
        printf("Member Count : %i\n",memberCount+1);
        if (*connection_fd < 0) { 
            printf("Client connection failed\n"); 
            exit(0); 
        }
        // printf("Client Successfully Connected\n");
        pthread_create(&clientHandlerThreads[memberCount], NULL, clientHandler ,(void *)connection_fd);
        printMembers();
    }
    // pthread_create(&write_thread, NULL, sendMsg, NULL);    

} 

void *clientHandler(void *socket_fd){
        int client_fd = *(int *)socket_fd;
        struct Register reg; 
        recv(client_fd, &reg,sizeof(reg),0);
        pthread_mutex_lock(&memberRegistrationMutex);
        if(memberCount >= MAX_MEMBERS){
            printf("Rejected Client %s. Maximum Capacity Reached.\n", reg.name);
            close(client_fd);
            pthread_cancel(clientHandlerThreads[memberCount]);
        }
        printf("%s Joined the Group!\n",reg.name);
        strcpy(members[memberCount], reg.name);
        members_socks[memberCount++] = client_fd;
        pthread_mutex_unlock(&memberRegistrationMutex);

        message recvMessage;
        while(recv(client_fd, &recvMessage,sizeof(recvMessage),0)) {
        // recv(connection_fd, recvBuffer, MAXLENGTH, 0);
            if(recvMessage.message_type){
                for(int j=0;j<memberCount;j++){
                    if(strcmp(members[j], recvMessage.sender) != 0){
                        send(members_socks[j], &recvMessage, sizeof(recvMessage),0); 
                    }
                }
            }
            else
            {
                int flag = 0;
                int from;
                for(int j=0;j<memberCount;j++){
                    if(strcmp(members[j], recvMessage.receiver) == 0){
                        send(members_socks[j], &recvMessage, sizeof(recvMessage),0); 
                        flag = 1;
                    }
                    if(strcmp(members[j], recvMessage.sender) == 0){
                        from = members_socks[j];
                    }
                }
                if(!flag)
                {
                    strcpy(not_found.message, "User not found\n");
                    send(from, &not_found, sizeof(not_found), 0);
                    // printf("No user registered with name %s\n", recvMessage.receiver);
                }
            }
        }
        
        printf("%s left the group\n",reg.name);
        pthread_mutex_lock(&memberRegistrationMutex);
        int exitedMember;
        for(int k=0;k<MAX_MEMBERS;k++){
            if(strcmp(reg.name, members[k]) == 0){
                exitedMember = k;
                break;
            }
        }
        for(exitedMember;exitedMember<memberCount;exitedMember++){
            members_socks[exitedMember] = members_socks[exitedMember + 1];
            strcpy(members[exitedMember],members[exitedMember + 1]);
        }
        memberCount--;
        pthread_mutex_unlock(&memberRegistrationMutex);
}

void terminate(int num){
    for(int i=0;i<MAX_MEMBERS;i++){
        pthread_kill(clientHandlerThreads[i],SIGKILL);
    }
    for(int i=0;i<MAX_MEMBERS;i++){
        close(members_socks[i]);
    }
    close(socket_fd);
    printf("\nClosing Socket. Exiting Application\n");
    exit(0);
}
  

void printMembers(void){
    pthread_mutex_lock(&memberRegistrationMutex);
    printf("Members : ");
    for(int k=0;k<memberCount+1;k++){
        printf("%s ",members[k]);
        fflush(stdout);
    }
    printf("\n");
    pthread_mutex_unlock(&memberRegistrationMutex);
}