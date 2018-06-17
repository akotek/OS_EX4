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

//TODO: 1. address of this server--> GetHostByNamePart() sys call?? lclhost??
//TODO: 2. Check in all exit() if sockets are CLOSED-SHOULD THEY??
//TODO: 3. delete all prints
//TODO: 4. what is MAX_buffer_size?
//TODO: 5. verify Error Handling (USAGE is missing)

// --------- Constants ---------
static const int MAX_CLIENTS = 50;
static const int MAX_PENDING_CONNECTIONS = 10;
static const int MAX_BUFFER_SIZE = 256;
static const int MIN_GROUP_NUM = 2;
static const std::regex portRegex("^([0-9]{1,"
"4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const string EXIT_CMD = "EXIT\n";
static const char* CLIENT_NAME_EXISTS = "Client name is already in use.\n";
static const char* FAILED_TO_CONNECT_SERVER = "Failed to connect the server";
static const char* CONNECTED_SUCCESSFULLY = "Connected Successfully.\n";
// -----------------------------


// --------- Data Structures ---------
std::map<string, int> clientToFdMap;
std::map<int, string> fdToClientMap;
std::map<string, std::vector<string>> groupsMap;
fd_set clientfds;   // Master fd of all sockets
struct sockaddr_in server; // Server to be binded later
const char* addr = {"127.0.0.1"};   // localhost
int serverSockfd; // Master socket file descriptor
int fdMax;  // maxFD number
// -------------------------------------


// --------- Program API: ---------
// Establishes server
void establishServer(const int &port);
// Closes all sockets of server
void shutdown();
// Connects new client
bool handleNewConnection(const int &sockfd);
// Handles client request
void handleClientRequest();
// Handles stdInput
bool handleStdInput();
// Removes client
void removeClient(const int &fd);
// Sets new connection
int setNewConnection();
// Handles a create_group request
void handleCreateGroupRequest(const int &fd, const string &groupName,
                              const vector<string> &clientsVec);
// Checks sys call return val
void checkSysCall(const int& sysCallVal, const string &sysCall);

// ------------------------------------

int main(int argc, char* argv[])
{
    cout << "Starting server on port: " << argv[1] << endl;
    // Input validation
    if (argc != 2 || !regex_match(argv[1], portRegex)){
        print_invalid_input();
        exit(-1);
    }

    // Establish server with master socket:
    int port = atoi(argv[1]);
    establishServer(port);


    fd_set readfds;   // Copy of master- used by select()
    FD_ZERO(&clientfds);
    FD_ZERO(&readfds);
    FD_SET(serverSockfd, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds);
    // Keep track of biggest file descriptor //TODO change this to numOfFds++?
    fdMax = serverSockfd;
    while (true){

        // Copy fds to selectSet
        readfds = clientfds;

        // We wait for any activity from select()
        int selectCall = select(fdMax+1, &readfds, NULL, NULL, NULL);
        checkSysCall(selectCall, "select");

        // Listen to masterSocket
        // If something happend on master socket,
        // Then we have a new connection from select():
        if (FD_ISSET(serverSockfd, &readfds))
        {
            // There is a new connection:
            int newSockfd = setNewConnection();
            if(handleNewConnection(newSockfd)){
                print_connection_server(fdToClientMap[newSockfd]);
            }
            if (newSockfd > fdMax)
            {
                fdMax = newSockfd;
            }
        }
        // Data is coming from stdin (server side):
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if(!handleStdInput()){
                exit(0);
            }
        }
        // We now check if there is any data from active sockets:
        else {
            //TODO
           // handleClientRequest(readfds);
            cout << "handle request" << endl;
        }
    } // END WHILE
    return 0;
}

int setNewConnection()
{
    int serverLen = sizeof(server);
    int newSockfd = accept(serverSockfd, (struct sockaddr *) &server, (socklen_t *) &serverLen);
    checkSysCall(newSockfd, "accept");
    return newSockfd;
}
bool handleNewConnection(const int& sockfd){
    cout << "in HandleNewConnection " << endl;

    char buf[MAX_BUFFER_SIZE] = {0};
    auto bytesRead = (int) read(sockfd, &buf, MAX_BUFFER_SIZE - 1);
    checkSysCall(bytesRead, "read");

    buf[bytesRead] = '\0';
    string clientName(buf);
    if (fdToClientMap.size() == MAX_CLIENTS){
        auto writeCall =  (int) write(sockfd, FAILED_TO_CONNECT_SERVER, strlen
                (FAILED_TO_CONNECT_SERVER));
        checkSysCall(writeCall, "write");
        return false;
    }
    if (clientToFdMap.count(clientName) || groupsMap.count(clientName)){
        // Client exists
        print_dup_connection();
        auto writeCall = (int) write(sockfd, CLIENT_NAME_EXISTS, strlen
                (CLIENT_NAME_EXISTS));
        checkSysCall(writeCall, "write");
        return false;
    }

    cout << "Adding new client..." << endl;
    FD_SET(sockfd, &clientfds);
    fdToClientMap[sockfd] = clientName;
    clientToFdMap[clientName] = sockfd;
    auto writeCall = (int) write(sockfd, CONNECTED_SUCCESSFULLY, strlen
            (CONNECTED_SUCCESSFULLY));
    checkSysCall(writeCall, "write");
    return true;
}

bool handleStdInput(){
    cout << "in handleStdInput()" << endl;

    char buf[MAX_BUFFER_SIZE] = {0};
    auto bytesRead = (int)read(STDIN_FILENO, &buf, MAX_BUFFER_SIZE - 1);
    checkSysCall(bytesRead, "read");

    buf[bytesRead] = '\0';
    if (!string(buf).compare(EXIT_CMD))
    {
        shutdown();
        print_exit();
        return false;
    }
    //TODO another thing was pressed in server shell- what to do in this case:
    return true;
}

void handleClientRequest(fd_set &readfds){
    cout << "in handleClientRequest" << endl;

    // We run on all active sockets
    // And check if anyone has got something for us:
    for (int fd = serverSockfd; fd < fdMax; fd++){
        char buf[MAX_BUFFER_SIZE] = {0};
        if (FD_ISSET(fd, &readfds)){
            auto bytesRead = (int)read(STDIN_FILENO, &buf, MAX_BUFFER_SIZE - 1);
            checkSysCall(bytesRead, "read");

            if (bytesRead == 0){
             // TODO User disconnected -- -- - - complete?
            }
            buf[bytesRead] = '\0';
            // Parse given command:
            string request(buf);
            command_type commandT;
            string name;
            string message;
            vector<string> clients;
            parse_command(request, commandT, name, message, clients);

            if (commandT == CREATE_GROUP){
                handleCreateGroupRequest(fd, name, clients);
                break;
            }
            else if (commandT == SEND){
               // handleSendRequest(...);
                break;
            }
            else if (commandT == WHO){
            //    handleWhoRequest();
                break;
            }
            else if (commandT == EXIT){
             //   handleExitRequest(fd);
                break;
            }

        }
    } // END for-loop
}

void handleCreateGroupRequest(const int &fd, const string &groupName,
                              const vector<string> &clientsVec)
{
    // Converts vector to set so no duplicates
    std::set<string> clientsSet(clientsVec.begin(), clientsVec.end());

    string clientName = fdToClientMap[fd];
    const bool isClientInGroup = clientsSet.find(clientName) != clientsSet.end();
    if (clientsVec.size() < MIN_GROUP_NUM || !isClientInGroup){
        print_create_group(true, false, clientName, groupName);
    }

    //TODO
    // Check that we have those clients in active_list:
    //const bool isInActive = ...getCurrentActiveClientsLex();

    // create group:
    groupsMap[clientName] = clientsVec;

    // msg client
    char msgToClient;
}

vector<string> getCurrentActiveClientsLex(){
    //TODO
}
void removeClient(const int &fd)
{
    cout << "in removeClient()" << endl;
    printf("Removes client with fd %d ", fd);
    close(fd);

    FD_CLR(fd, &clientfds);
    //TODO removeFromGroup(fdToClientMap[fd]);
    clientToFdMap.erase(fdToClientMap[fd]);
    fdToClientMap.erase(fd);
}

void shutdown(){
    cout << "Shutting down: closing " << fdMax << " sockets" << endl;
    for(int i = serverSockfd; i< fdMax; i++){
        close(i);
    }
}

int createSocket(){
    int socketFd = socket(AF_INET , SOCK_STREAM , 0);
    checkSysCall(socketFd, "socket");
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

void bindSocket(const int &socketFd, struct sockaddr_in &server){
    int bindCall = bind(socketFd, (struct sockaddr *)&server , sizeof(server));
    checkSysCall(bindCall, "bind");
}

void listenSocket(const int &socketFd){
    int listenCall = listen(socketFd, MAX_PENDING_CONNECTIONS);
    checkSysCall(socketFd, "listen");
}

void establishServer(const int &port){
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

void checkSysCall(const int& sysCallVal, const string &sysCall){
    if (sysCallVal < 0){
        print_error(sysCall, errno);
        exit(1);
    }
}
