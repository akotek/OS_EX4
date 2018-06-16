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
int serverSockfd; // Master socket file descriptor
// ====================================


// ========== Main program Functions ==========
// Establishes server
int establishServer(int &port);
// Checks for system call and closes socket
void terminateServer(const int *socket = nullptr, const char* errorMsg);
// Connects new client
void connectNewClient(int &sockfd);

// ============================================

void handleClientRequest(char &data);

int main(int argc, char* argv[])
{

    // Input :: whatsappServer portNum
    // Validated in client side
    if (argc != 2){
        print_invalid_input();
        print_exit();
        exit(-1);
    }

    // Establish server:
    int port = atoi(argv[2]);
    int serverLen = establishServer(port);

    // Socket descriptors
    // WE need a different set for select
    fd_set clientfds;   // Master fd of all sockets
    fd_set readfds;   // Copy of master- used by select()
    int fdMax;  // maxFD number

    // Clear sockets:
    FD_ZERO(&clientfds);
    FD_ZERO(&readfds);

    // Add master socket to set:
    // Add STDIN fd
    FD_SET(serverSockfd, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds);
    socketsMap["server"] = serverSockfd;

    // Keep track of biggest file descriptor
    fdMax = serverSockfd;

    while (true){

        // Copy fds to selectSet
        readfds = clientfds;

        // We wait for any activity from Select() sysCall
        int activityFromSelect = select(fdMax+1, &readfds, NULL, NULL, NULL);
        if (activityFromSelect < 0) {
            terminateServer(&serverSockfd, "select");
            exit(-1);
        }

        // Run on all existing connections (sockets)
        // If we got active socket - read it request
        for (int i =0; i < fdMax; i++){ //TODO start from MasteRSocketFD?

            // Check if fd is updated by select
            // (if there is a socket-connection):
            if (FD_ISSET(i, &readfds)) // Returns nonzero if in our set
            {
                // There is a new connection:
                if (i == serverSockfd)
                {
                    int newSockfd = accept(serverSockfd, (struct sockaddr *)
                            &server, (socklen_t *) &serverLen);
                    if (newSockfd < 0)
                    {
                        terminateServer(&serverSockfd, "accept");
                        exit(-1);
                    }
                    // Connect new client:
                    FD_SET(newSockfd, &clientfds); // Add newSock to masterSet
                    connectNewClient(newSockfd);
                    if (newSockfd > fdMax)
                    {
                        fdMax = newSockfd;
                    } //
                    puts("New connection");
                }
                else
                {
                    // Handle data from client:
                    // Read his data :: check if disconnected/null
                    char dataFromClient[256];
                    int numOfBytesRead;
                    if ((numOfBytesRead = recv(i, dataFromClient, sizeof(dataFromClient), 0)) <= 0){
                        // Error or connection closed by client:
                        if (numOfBytesRead == 0){
                            // no bytes read connection closed
                            // i == cur_fd
                            printf("Select: socket %d closed connection\n", i);
                        } else {
                            // sysCall error
                            terminateServer(&i, "recv");
                            exit(-1);
                        }
                        close(i); // close socket
                        FD_CLR(i, &clientfds); // remove socket
                        //todo socketsMap.erase(byFD erase....)
                    } else { // Recieved data from client:
                        // Check if client is in readfds
                        // and recieve a msg from him
                        dataFromClient[numOfBytesRead] = '\0';
                        handleClientRequest(*dataFromClient);

                    }
                } // END handle data from client
            } // END new connection
        } // END loop all over our sockets
    } // END main loop

    return 0;
}

void handleClientRequest(const char &data){
    // Parse client request and do some logic:
    // Switch CASE
    // untill we see '\0'\

    // use send()...... to send data

}
void connectNewClient(int& sockfd) {

}





void terminateServer(const int *socket = nullptr, const char* errorMsg){
    if (socket) close(*socket); // If socket not null- close it
    // todo perror recieves syscall and will print it erro
    // todo need to verify it works with error handling in ex
    perror(errorMsg);
    printf("Error in %s () \n", errorMsg);
    print_exit();
}

int createSocket(){
    int socketFd = socket(AF_INET , SOCK_STREAM , 0);
    if (socketFd < 0) {
        terminateServer(&socketFd, "socket");
        exit(-1);
    }
    return socketFd;
}

void initServer(const int& port, const char* addr, struct sockaddr_in
&server)
{
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(addr);
    memset(&(server.sin_zero), '\0', 8);
}

void bindSocket(int &socketFd, struct sockaddr_in &server){
    if(bind(socketFd, (struct sockaddr *)&server , sizeof(server)) < 0) {
        terminateServer(&socketFd, "bind");
        exit(-1);
    }
}

void listenSocket(int &socketFd){
    if(listen(socketFd, MAX_PENDING_CONNECTIONS) < 0){
        terminateServer(&socketFd, "listen");
        exit(-1);
    }
}

int establishServer(int &port){
    // Creates a masterSocket
    // And init's server:
    serverSockfd = createSocket();
    initServer(port, addr, server);
    printf("Socket created and server init succeeded \n");

    // Bind masterSocketFd to Server
    bindSocket(serverSockfd, server);
    printf("Binding succeded, listening on port %d \n", port);

    // Now masterSocket will listen to new arrivals:
    listenSocket(serverSockfd);
    int serverlen = sizeof(server);
    printf("Waiting for incoming connections...\n");
    return serverlen;
}
