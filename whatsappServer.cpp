#include "whatsappio.h"
#include <iostream>
#include <map>
#include <set>
#include<cstdio>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>
using namespace std;

// =========== Constants ===========
const int MAX_CLIENTS = 50;
const int MAX_PENDING_CONNECTIONS = 10;
// =================================

// ========== Data Structures ==========
std::map<char*, int> socketsMap; // K: clientName, V: FD
std::map<char*, std::set<char*>> groupsMap; // K: gName, V: setOfClients;
struct sockaddr_in server; // Server to be binded later
const char* addr = {"127.0.0.1"};   // localhost
int masterSocketFd; // Master socket FD, for n clients -> n+1 fd's
// ====================================


void checkSysCall(int &sysCallValue){
    if (sysCallValue < 0){
        // TODO print error
        print_exit();
        exit(-1);
    }
}
int createSocket(){
    int socketFd = socket(AF_INET , SOCK_STREAM , 0);
    checkSysCall(socketFd);
    return socketFd;
}

void initServer(const int& port, const char* addr, struct sockaddr_in
                &server){
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(addr);
    memset(&(server.sin_zero), '\0', 8);

void bindSocket(int &socketFd, struct sockaddr_in &server){
    int bindSysCall = bind(socketFd, (struct sockaddr *)&server , sizeof(server));
    if (bindSysCall < 0) {
        close(socketFd); // close socket
        //TODO print error
        puts("error in binding socket");
        print_exit();
        exit(-1);
    }
}

void listenSocket(int &socketFd){
    int listenSysCall = listen(socketFd, MAX_PENDING_CONNECTIONS);
    if (listenSysCall < 0){
        close(socketFd);
        //TODO print error
        puts("error in listen socket");
        print_exit();
        exit(-1);
    }
}

void establishServer(int &port){
    // Create masterSocket
    // And init server:
    masterSocketFd = createSocket();
    initServer(port, addr, server);
    printf("Socket created and server init succeeded \n");

    // Bind masterSocketFd to Server
    bindSocket(masterSocketFd, server);
    printf("Binding succeded, listening on port %d \n", port);

    // Now masterSocket will listen to new arrivals:
    listenSocket(masterSocketFd);
    int serverlen = sizeof(server);
    printf("Waiting for incoming connections...\n");
}

int main(int argc, char* argv[])
{

    // Input :: whatsappServer portNum
    // Validated in client side
    if (argc != 2){
        printf("Input is invalid - wrong num of args \n");
        print_exit();
        exit(-1);
    }

    fd_set clientfds; // Socket descriptors
    fd_set readfds;

    // Establish server:
    int port = atoi(argv[2]);
    establishServer(port);







    return 0;
}//