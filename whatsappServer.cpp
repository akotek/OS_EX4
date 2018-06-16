#include <string>
#include "whatsappio.h"
#include <iostream>
#include <map>
#include <set>
#include <netinet/in.h>
#include <regex>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

//TODO: 1. make small main() -> split to funcs
//TODO: 2. address of this server--> is it localhost?
//TODO: 3. Check in all exit() if sockets are CLOSED-SHOULD THEY??

// =========== Constants ===========
static const int MAX_CLIENTS = 50;
static const int MAX_PENDING_CONNECTIONS = 10;
static const int MAX_BUFFER_SIZE = 256;
static const std::regex portRegex("^([0-9]{1,"
"4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const string EXIT_CMD = "EXIT\n";
// =================================


// ========== Data Structures ==========
std::map<char*, int> clientToFdMap;
std::map<int, char*> fdToClientMap;
std::map<char*, std::set<char*>> groupsMap; // K: gName, V: setOfClients;
struct sockaddr_in server; // Server to be binded later
const char* addr = {"127.0.0.1"};   // localhost
int serverSockfd; // Master socket file descriptor
int fdMax;  // maxFD number
// ====================================


// ========== Program API: ==========
// Establishes server
void establishServer(int &port);
// Closes all sockets of server
void shutdown();
// Connects new client
void connectNewClient(int &sockfd);
// Handles client request
void handleClientRequest(char &data);
// Handles stdInput
void handleStdInput();
// Prints system call Errr's with errNum
void printSysCallError(const string &sysCall, const int errNum);

// ==================================

void removeClient(int &fd, fd_set &clientfds);

int main(int argc, char* argv[])
{
    cout << argc << endl;
    // Input validation
    if (argc != 3 || !regex_match(argv[2], portRegex)){
        print_invalid_input();
        exit(-1);
    }

    // Establish server with master socket:
    int port = atoi(argv[2]);
    establishServer(port);

    // Socket descriptors
    // WE need a different set for select
    fd_set clientfds;   // Master fd of all sockets
    fd_set readfds;   // Copy of master- used by select()

    // Clear sockets,
    // Add master socket and STDIN to set:
    FD_ZERO(&clientfds);
    FD_ZERO(&readfds);
    FD_SET(serverSockfd, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds);

    // Keep track of biggest file descriptor
    fdMax = serverSockfd;
    int serverLen = sizeof(server);
    while (true){

        // Copy fds to selectSet
        readfds = clientfds;

        // We wait for any activity from Select() sysCall
        int activityFromSelect = select(fdMax+1, &readfds, NULL, NULL, NULL);
        if (activityFromSelect < 0) {
            printSysCallError("select", errno);
            exit(-1);
        }

        // Listen to masterSocket
        // If something happend on master socket,
        // Then we have a new connection from select():
        if (FD_ISSET(serverSockfd, &readfds)) // Returns nonzero if in our set
        {
            // There is a new connection:
            int newSockfd = accept(serverSockfd, (struct sockaddr *)
                    &server, (socklen_t *) &serverLen);
            if (newSockfd < 0)
            {
                printSysCallError("accept", errno);
                exit(-1);
            }
            // Connect new client:
            //TODO this not goooood
            FD_SET(newSockfd, &clientfds); // Add newSock to masterSet
            connectNewClient(newSockfd);
            if (newSockfd > fdMax)
            {
                fdMax = newSockfd;
            }
            puts("New connection");
        }
        char buf[MAX_BUFFER_SIZE] = {0};
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            handleStdInput();
        }

        else {
            for(int fd = serverSockfd+1; fd < fdMax; fd++){
                if (FD_ISSET(fd, &readfds)){
                    // Handle data from client,
                    // Check if disconnected/valid:
                    int bytesRead = (int)read(fd, &buf, MAX_BUFFER_SIZE -
                            1);;
                    if (((bytesRead)) <= 0){
                        // Error or connection closed by client:
                        if (bytesRead == 0){
                            // no bytes read connection closed
                            printf("Select: socket %d closed connection\n", fd);
                        } else {
                            printSysCallError("read", errno);
                            exit(-1);
                        }
                        // Close and remove socket from all DS:
                        removeClient(fd, clientfds);

                    } else { // Recieved data from client:
                        // Check if client is in readfds
                        // and recieve a msg from him
                        buf[bytesRead] = '\0';
                        string request(buf); // convert char to str
                        //handleClientRequest(request);
                    }
                }
            }
        }
    }
    return 0;
}

void removeClient(int &fd, fd_set &clientfds)
{
    close(fd);
    FD_CLR(fd, &clientfds);
    clientToFdMap.erase(fdToClientMap[fd]);
    fdToClientMap.erase(fd);
}

void handleStdInput(){
    char buf[MAX_BUFFER_SIZE];

    cout << "in handleStdInput()" << endl;
    int bytesRead = (int)read(STDIN_FILENO, &buf, MAX_BUFFER_SIZE - 1);
    if(bytesRead < 0){
        printSysCallError("read", errno);
        exit(1);
    }
    buf[bytesRead] = '\0';
    if (!string(buf).compare(EXIT_CMD))
    {
        shutdown();
        print_exit();
        exit(0);
    }
}

void handleClientRequest(const string &request){
    cout << "in handleClientRequest" << request << endl;
    // Parse client request and do some logic:
    // Switch CASE

    // use send()...... to send data

}
void connectNewClient(int& sockfd) {

    cout << "in connectNewClient with socket num: " << sockfd << endl;

}

void printSysCallError(const string &sysCall, const int errNum){
    cerr << "ERROR: " << sysCall << " " <<  errNum << endl;
}

void shutdown(){
    cout << "Shutting down: closing " << fdMax << " sockets" << endl;
    for(int i =3; i< fdMax; i++){
        close(i);
    }
}

int createSocket(){
    int socketFd = socket(AF_INET , SOCK_STREAM , 0);
    if (socketFd < 0) {
        printSysCallError("socket", errno);
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
        printSysCallError("bind", errno);
        exit(-1);
    }
}

void listenSocket(int &socketFd){
    if(listen(socketFd, MAX_PENDING_CONNECTIONS) < 0){
        printSysCallError("listen", errno);
        exit(-1);
    }
}

void establishServer(int &port){
    // Creates a masterSocket
    // And init's server:
    serverSockfd = createSocket();
    initServer(port, addr, server);
    printf("Socket created and server init succeeded \n");

    bindSocket(serverSockfd, server);
    printf("Binding succeded, listening on port %d \n", port);

    listenSocket(serverSockfd);
    printf("Waiting for incoming connections...\n");
}
