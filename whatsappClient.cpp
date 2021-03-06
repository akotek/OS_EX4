//
// Created by yoavabadi on 6/15/18.
//
#include <netdb.h>
#include "whatsappio.h"
#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <cctype>    // for std::isalnum
#include <regex>
#include <termios.h>

using namespace std;


// --------- Constants ---------
#define VALID_ARGC 4
#define EMPTY_STRING ""
#define MIN_STR 2
#define SOCKETS_NUM 4
#define EXIT_STR "exit"
#define CLOSE_SOCKET_MSG "close"
static const int MAX_BUFFER_SIZE = 256;

static const std::regex portRegex("^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const std::regex ipRegex("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\."
                                        "[0-9]{1,3}$");
static const std::regex commaRegex(".*(,,|, ,).*");
static const std::regex spaceRegex("(\\s)*");
static const std::regex invalidSendRegex("^send\\s[a-zA-Z0-9]*$");
string const CLIENT_NAME_EXISTS = "duplicate";

// --------- Globals ---------
const char* clientName;
const char* server_ip;
int server_port;

struct sockaddr_in server_sockaddr;
struct hostent *hp;
int clientFd;
fd_set readFds;

void checkSysCall(const int& sysCallVal, const string &sysCall)
{
    if (sysCallVal < 0)
    {
        print_error(sysCall, errno);
        exit(1);
    }
}


bool all_alphanumeric(const std::string& s)
{
    return all_of(s.begin(),s.end(),
                       [](char c) { return isalnum(c); });
}

// --------- Client API: ---------


void validateArguments(int argc, char* argv[]);

// ------------------------------------


void validateArguments(int argc, char* argv[])
{
    // Input validation
    if (argc != VALID_ARGC || !all_alphanumeric(argv[1]) ||
            string(argv[1]).size() > MAX_BUFFER_SIZE ||
        !regex_match(argv[2], ipRegex) ||  !regex_match(argv[3], portRegex))
    {
        print_client_usage();
        exit(1);
    }

    // Input assignment
    clientName = argv[1];
    server_ip = argv[2];
    server_port = atoi(argv[3]);
}


void establishConnection()
{
    if ((hp = gethostbyname(server_ip)) == NULL)
    {
        print_fail_connection();
        exit(1);
    }
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    memcpy((char *)&server_sockaddr.sin_addr, hp->h_addr, hp->h_length);
    server_sockaddr.sin_family = hp->h_addrtype;
    server_sockaddr.sin_port = htons((u_short)server_port);
    // Build the socket
    clientFd = (int)socket(hp->h_addrtype, SOCK_STREAM, 0);
    checkSysCall(clientFd, "socket");
    // Connect to server
    auto connectionCheck = (int)connect(clientFd, (struct sockaddr *)
                             &server_sockaddr, sizeof(server_sockaddr));
    if (connectionCheck < 0)
    {
        close(clientFd);
        print_fail_connection();
        exit(1);
    }
    // Register clientName to server
    auto registerCheck = (int)write(clientFd, clientName, strlen(clientName));
    if (registerCheck < 0)
    {
        print_fail_connection();
        exit(1);
    }

    // Read response from server
    char buf[MAX_BUFFER_SIZE] = {0};
    auto bytesRead = (int) read(clientFd, &buf, MAX_BUFFER_SIZE - 1);
    checkSysCall(bytesRead, "read");
    buf[bytesRead] = '\0';
    string serverResponse(buf);

    if(serverResponse == CLIENT_NAME_EXISTS)
    {
        print_dup_connection();
        exit(1);
    }
    print_connection();
}


bool validateInput(const string &command)
{

    // inputs to insert to the parse function
    command_type commandT;
    string name;
    string message;
    vector<string> clients;

    if(regex_match(command, invalidSendRegex))
    {
        print_invalid_input();
        return false;
    }

    // splits user input to workable pieces
    parse_command(command, commandT, name, message, clients);

    // validate user input by command type:
    switch (commandT)
    {
        case INVALID:
        {
            print_invalid_input();
            return false;
        }

        case SEND:
        {
            if (name == clientName || regex_match(message, spaceRegex))
            {
                print_send(false, false, clientName, name, command);
                return false;
            }
            break;
        }

        case CREATE_GROUP:
        {
            // check if group_name is alphanumeric &
            // there is more then 1 client
            if(!all_alphanumeric(name)) //  || clients.size() < 2
            {
                print_create_group(false, false, clientName, name);
                return false;
            }

            // example: a,, ,, input
            if(clients.empty() || regex_match(command, commaRegex))
            {
                print_create_group(false, false, clientName, name);
                return false;
            }

            // checks the client names are valid
            for(string& client : clients)
            {
                if(!all_alphanumeric(client))
                {
                    print_create_group(false, false, clientName, name);
                    return false;
                }
            }
            break;
        }

        case WHO:
        {
            if(command != "who")
            {
                print_invalid_input();
                return false;
            }
            break;
        }

        case EXIT:
        {
            if(command != "exit")
            {
                print_invalid_input();
                return false;
            }
            break;
        }
    }
    return true;
}


/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[])
{
    fd_set clientListenFds; // std in and server socket
    // Input validation && assignment
    validateArguments(argc, argv);
    // create a connection to the server
    establishConnection();
    FD_ZERO(&readFds);
    FD_SET(STDIN_FILENO, &readFds);
    FD_SET(clientFd, &readFds);


    while(true)
    {
        char readBuffer[MAX_BUFFER_SIZE] = {0};
        string command;
        clientListenFds = readFds;
        auto listen = (int)select(SOCKETS_NUM , &clientListenFds, NULL, NULL, NULL);

        checkSysCall(listen, "select");

        // got command from client
        if (FD_ISSET(STDIN_FILENO, &clientListenFds))
        {
            auto inRead = (int)read(STDIN_FILENO, readBuffer,
                                                     MAX_BUFFER_SIZE -1);

            checkSysCall(inRead, "read");
            // parse_command errors out with empty string
            if(inRead < 0 || string(readBuffer) == EMPTY_STRING ||
                    string(readBuffer).size() < MIN_STR)
            {
                print_invalid_input();
                continue;
            }
            command = string(readBuffer);
            command = command.substr(0, command.size()-1); // removes newline

            if(!validateInput(command))
            {
                continue;
            }

            auto writeToServer = (int)write(clientFd, command.c_str(),
                                            strlen(command.c_str()) + 1);
            checkSysCall(writeToServer, "write");

            if(command == EXIT_STR)
            {
                print_exit(false, clientName);
                exit(0);
            }

        }
        // got message from server
        else if (FD_ISSET(clientFd, &clientListenFds))
        {

            // Read response from server
            char buf[MAX_BUFFER_SIZE] = {0};
            auto bytesRead = (int)read(clientFd, &buf, MAX_BUFFER_SIZE - 1);
            checkSysCall(bytesRead, "read");

            buf[bytesRead] = '\0';
            string serverResponse(buf);
            if(serverResponse == CLOSE_SOCKET_MSG)
            {
                print_exit(false, clientName);
                exit(0);
            }

            tcflush(clientFd, TCIOFLUSH);
            cout << serverResponse << endl;
        }

    }
    return 0;
}