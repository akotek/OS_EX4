#include <string>
#include "whatsappio.h"
#include <iostream>
#include <map>
#include <set>
#include <netinet/in.h>
#include <regex>
#include <arpa/inet.h>
#include <unistd.h>
#include <termio.h>
using namespace std;

//TODO: 1. address of this server--> GetHostByNamePart() sys call?? lclhost??
//TODO: 2. Check in all exit() if sockets are CLOSED-SHOULD THEY??
//TODO: 3. delete all prints
//TODO: 4. what is MAX_buffer_size?
//TODO: 5. verify Error Handling (USAGE is missing)
//
//In case of a system call error in the server side, print “ERROR: <system call function name>
//<errno>.\n” (for example: “ERROR: accept 22.\n”)


// --------- Constants ---------
static const int MAX_CLIENTS =  50;
static const int MAX_PENDING_CONNECTIONS = 10;
static const int MAX_BUFFER_SIZE = 256;
static const int MIN_GROUP_NUM = 2;
static const std::regex portRegex("^([0-9]{1,"
                                          "4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const string EXIT_CMD = "EXIT\n";
static const char *const CLIENT_NAME_EXISTS = "duplicate";
static const char* FAILED_TO_CONNECT_SERVER  = "Failed to connect the server";
static const char* CONNECTED_SUCCESSFULLY = "Connected Successfully.\n";
static const char* UNREGISTERED_SUCCESSFULLY_MSG =
        "Unregistered successfully.\n";
#define SEND_ERROR_MSG "ERROR: failed to send "
#define CREATE_GROUP_ERROR_MSG "ERROR: failed to create group "
#define SEND_SUCCESS_MSG "was sent successfully to "
static const char *const CLOSE_SOCKET_MSG = "close";

//
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


// --------- Server's APIs: ---------

// Establishes server
void establishServer(const int &port);
// Closes all sockets of server
void shutdown();
// Connects new client
bool handleNewConnection(const int &sockfd);
// Handles client request
void handleClientRequest(fd_set &readfds);
// Handles stdInput
bool handleStdInput();
// Sets new connection
int setNewConnection();
// Handles a create_group request
void handleCreateGroupRequest(const string &clientName, const string &groupName,
                              vector<string> &clientsVec);
// Checks sys call return val
void checkSysCall(const int& sysCallVal, const string &sysCall);
// Handles a send request
void handleSendRequest(const string & clientName, const string &name,
                       const string &message);
void handleWhoRequest(const int clientFd);
void handleExitRequest(const int clientFd);
void handlePeerToPeerMessage(const string &clientName, const string &peerName,
                             const string &serverMessage);
// ------------------------------------

int main(int argc, char* argv[])
{
//    cout << "Starting server on port: " << argv[1] << endl;
    // Input validation
    if (argc != 2 || !regex_match(argv[1], portRegex)){
        print_server_usage();
        exit(-1);
    }

    // Establish server with master socket:
    int port = atoi(argv[1]);
    establishServer(port);


    fd_set readfds;   // Copy of master- used by select()
    FD_ZERO(&clientfds);
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &clientfds);
    FD_SET(serverSockfd, &clientfds);
    // Keep track of biggest file descriptor
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
            handleClientRequest(readfds);
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
        auto writeCall = (int) write(sockfd, CLIENT_NAME_EXISTS, strlen
                (CLIENT_NAME_EXISTS));
        checkSysCall(writeCall, "write");
        return false;
    }

    FD_SET(sockfd, &clientfds);
    fdToClientMap[sockfd] = clientName;
    clientToFdMap[clientName] = sockfd;

    auto writeCall = (int) write(sockfd, CONNECTED_SUCCESSFULLY, strlen
            (CONNECTED_SUCCESSFULLY));
    checkSysCall(writeCall, "write");
    return true;
}


bool handleStdInput(){

    char buf[MAX_BUFFER_SIZE] = {0};
    auto bytesRead = (int)read(STDIN_FILENO, &buf, MAX_BUFFER_SIZE - 1);
    checkSysCall(bytesRead, "read");

    buf[bytesRead] = '\0';
    if (string(buf) == EXIT_CMD)
    {
        shutdown();
        print_exit();
        close(serverSockfd);
        return false;
    }
    // If something else was pressed: don't do anything
    return true;
}

void handleClientRequest(fd_set &readfds){
    // We run on all active sockets
    // And check if anyone has got something for us:
    for (auto& mapPair : fdToClientMap){
        int fd = mapPair.first;
        char buffer[MAX_BUFFER_SIZE];
        if (FD_ISSET(fd, &readfds)){
            auto bytesRead = (int)read(fd, &buffer, MAX_BUFFER_SIZE -1);
            checkSysCall(bytesRead, "read");
            tcflush(fd, TCIOFLUSH);

            if (bytesRead == 0){
                // User disconnected:
                // In drill it says it will only disconnect with 'exit'
                return;
            }
            buffer[bytesRead] = '\0';
            // Parse given command:
            string request(buffer);
            command_type commandT;
            string name;
            string message;
            vector<string> clients;
            parse_command(request, commandT, name, message, clients);

            string clientName = fdToClientMap[fd];
            if (commandT == CREATE_GROUP){
                handleCreateGroupRequest(clientName, name, clients);
                break;
            }
            else if (commandT == SEND){
                handleSendRequest(clientName, name, message);
                break;
            }
            else if (commandT == WHO)
            {
                handleWhoRequest(fd);
                break;
            }
            else if (commandT == EXIT)
            {
                handleExitRequest(fd);
                break;
            }
        }
    }
}


void handleExitRequest(const int clientFd)
{
    string clientName = fdToClientMap[clientFd];

    clientToFdMap.erase(clientName);
    fdToClientMap.erase(clientFd);

    // Remove client from groups.
    for(auto& group : groupsMap)
    {
        vector<string>::iterator deleteIter;
        for(auto member = group.second.begin();
            member != group.second.end(); member++)
        {
            if((*member) == clientName)
            {
                deleteIter = member;
                group.second.erase(deleteIter);
                break;
            }
        }
        if (group.second.empty()){
            groupsMap.erase(group.first);
        }
    }

    auto writeCall = (int) write(clientFd, UNREGISTERED_SUCCESSFULLY_MSG,
                                 strlen(UNREGISTERED_SUCCESSFULLY_MSG));
    checkSysCall(writeCall, "write");
    close(clientFd);
    FD_CLR(clientFd, &clientfds);

    print_exit(true, clientName);
}


bool validateGroupMembers(vector<string> groupMembers, string groupName,
                          string clientName){

    // check if groupName already exists as another group or user name
    if (!( groupsMap.find(groupName) == groupsMap.end()) ||
            !( clientToFdMap.find(groupName) == clientToFdMap.end()))
    {
        return false;
    }

    // check if current client in the group he wants to create
    if (!(find(groupMembers.begin(), groupMembers.end(), clientName)
          != groupMembers.end()))
    {
        return false;
    }
    // check that all group members are connected to the server
    for(auto member = groupMembers.begin(); member != groupMembers.end();
        member++)
    {
        if(clientToFdMap[(*member)] < 2 || clientToFdMap[(*member)] > fdMax)
        {
            return false;
        }
    }

    return true;
}
//
void handleCreateGroupRequest(const string &clientName, const string &groupName,
                              vector<string> &clientsVec)
{
    clientsVec.push_back(clientName);
    // Converts vector to set so no duplicates
    std::set<string> clientsSet(clientsVec.begin(), clientsVec.end());

    // Check that we have those clients in active_list:
    const bool isInActive = validateGroupMembers(clientsVec,
                                                 groupName, clientName);
    if (clientsSet.size() < MIN_GROUP_NUM || !isInActive
        ){
        print_create_group(true, false, clientName, groupName);

        // send error to client
        string errorMessage = string(CREATE_GROUP_ERROR_MSG) +
                              "\"" + groupName + "\".\n";

        auto sysCall = (int)write(clientToFdMap[clientName],
                                  errorMessage.c_str(),
                                  strlen(errorMessage.c_str()));
        checkSysCall(sysCall, "write");
        return;
    }


    // create group:
    groupsMap[groupName] = clientsVec;
    print_create_group(true, true, clientName, groupName);


    // msg client
    string serverMessage = "Group \"" + groupName
                           + "\" was created successfully.";
    auto sysCall = (int)write(clientToFdMap[clientName],
                              serverMessage.c_str(),
                              strlen(serverMessage.c_str()));
    checkSysCall(sysCall, "write");
}


void handleGroupMessage(const string &clientName, const string &groupName,
                        const string &serverMessage)
{

    vector <string> groupMembers = groupsMap[groupName];
    for (auto &groupMember : groupMembers)
    {
        if(groupMember != clientName)
        {
            auto sysCall = (int)write(clientToFdMap[groupMember],
                                      serverMessage.c_str(),
                                      strlen(serverMessage.c_str()));
            checkSysCall(sysCall, "write");
        }
    }
}

void sendSuccessMessages(const string &clientName, const string &name,
                         const string &message)
{
    print_send(true, true, clientName, name, message);

    // send error to client

    string successMessage = "Sent successfully.";
    auto sysCall = (int)write(clientToFdMap[clientName],
                              successMessage.c_str(),
                              strlen(successMessage.c_str()));
    checkSysCall(sysCall, "write");
}

void handlePeerToPeerMessage(const string &clientName, const string &peerName,
                             const string &serverMessage)
{
    auto sysCall = (int)write(clientToFdMap[peerName],
                              serverMessage.c_str(),
                              strlen(serverMessage.c_str()));
    checkSysCall(sysCall, "write");
}


bool isInGroup(const string& clientName, const string& groupName)
{

    for (auto& group : groupsMap)
    {
        if(group.first == groupName && (find(group.second.begin(),
           group.second.end(), clientName) != group.second.end()))
        {
            return true;
        }
    }
    return false;
}


void handleSendRequest(const string &clientName, const string &name,
                       const string &message)
{

    string serverMessage = clientName + ": " + message;

    // Handle send to group request
    if (!( groupsMap.find(name) == groupsMap.end())
        && isInGroup(clientName, name))
    {
        handleGroupMessage(clientName, name, serverMessage);
        sendSuccessMessages(clientName, name, message);
    }
        // Handle send to another client request
    else if (!( clientToFdMap.find(name) == clientToFdMap.end()))
    {
        handlePeerToPeerMessage(clientName, name, serverMessage);
        sendSuccessMessages(clientName, name, message);
    }
        // If name not a client or a group name - error
    else
    {
        print_send(true, false, clientName, name, message);
        // send error to client
        string errorMessage = string(SEND_ERROR_MSG) +
                              "\"" + message + "\" to " + name + ".";

        auto sysCall = (int)write(clientToFdMap[clientName],
                                  errorMessage.c_str(),
                                  strlen(errorMessage.c_str()));
        checkSysCall(sysCall, "write");
    }
}


void handleWhoRequest(const int clientFd)
{
    string whoMessage;
    static const auto addToString =
            [&whoMessage](const string& client)
            { whoMessage += client + ",";};

    // Converts vector to set so it will be ordered
    std::set<string> clientsSet;

    for (auto &client : fdToClientMap)
    {
            clientsSet.insert(client.second);
    }

    if(clientsSet.empty())
    {
        // TODO handle empty client list
    }
    // set to string
    std::for_each(clientsSet.begin(), clientsSet.end(), addToString);
    // remove last ","
    whoMessage = whoMessage.substr(0, whoMessage.size() - 1) + ".";

    // Send response and server message
    print_who_server(fdToClientMap[clientFd]);
    auto sysCall = (int)write(clientFd,
                              whoMessage.c_str(),
                              strlen(whoMessage.c_str()));
    checkSysCall(sysCall, "write");
}


void shutdown(){

    for(const auto client : fdToClientMap)
    {
        auto sendCheck = (int)write(client.first, CLOSE_SOCKET_MSG, strlen
                (CLOSE_SOCKET_MSG));
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
    bindSocket(serverSockfd, server);
    listenSocket(serverSockfd);
}

void checkSysCall(const int& sysCallVal, const string &sysCall){
    if (sysCallVal < 0){
        print_error(sysCall, errno);
        exit(1);
    }
}