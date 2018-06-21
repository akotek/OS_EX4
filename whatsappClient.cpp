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
static const int MAX_BUFFER_SIZE = 256;

static const std::regex portRegex("^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const std::regex ipRegex("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\."
                                        "[0-9]{1,3}$");
string const CLIENT_NAME_EXISTS = "duplicate";

// --------- Globals ---------
const char* clientName;
const char* server_ip;
int server_port;

struct sockaddr_in server_sockaddr;
struct hostent *hp;
int clientFd, valread;
fd_set readFds;
//struct sockaddr_in serv_addr;


bool all_alphanumeric(const std::string& s)
{
    string const commands[] = {"send", "exit", "who", "create_group"};
    for(const string& c : commands)
    {
        if(c == s) return false;
    }
    return all_of(s.begin(),s.end(),
                       [](char c) { return isalnum(c); });
}

// --------- Client API: ---------

//void handleGroupValidation()
//{
//
//}
//


void validateArguments(int argc, char* argv[]);

// ------------------------------------


void validateArguments(int argc, char* argv[])
{
    // Input validation
    if (argc != VALID_ARGC || !all_alphanumeric(argv[1]) ||
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


// TODO: change and cleanup
void establishConnection()
{

    if ((hp = gethostbyname(server_ip)) == NULL)
    {
//        cerr << FAILED_CONNECT_MESSAGE << endl << flush; //TODO: error msg
        exit(1);
    }

    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    memcpy((char *)&server_sockaddr.sin_addr, hp->h_addr, hp->h_length);
    server_sockaddr.sin_family = hp->h_addrtype;
    server_sockaddr.sin_port = htons((u_short)server_port);
    // build the socket
    if ((clientFd = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        exit(1);
    }
    // Connect to server
    if (connect(clientFd, (struct sockaddr *)&server_sockaddr,
                sizeof(server_sockaddr)) < 0)
    {
        close(clientFd);
        print_fail_connection();
        exit(1);
    }
    // Register clientName to server
    if (write(clientFd, clientName, strlen(clientName)) < 0)
    {
        print_fail_connection(); // TODO: check what should be the error message
        exit(1);
    }

    // Read response from server
    char buf[MAX_BUFFER_SIZE] = {0};
    auto bytesRead = (int) read(clientFd, &buf, MAX_BUFFER_SIZE - 1);
    buf[bytesRead] = '\0';
    string serverResponse(buf);

    if(serverResponse == CLIENT_NAME_EXISTS)
    {
        print_dup_connection();
        exit(1);
    }

    cout << serverResponse << endl;
}


bool validateInput(const string &command)
{


    // inputs to insert to the parse function
    command_type commandT;
    string name;
    string message;
    vector<string> clients;

    // splits user input to workable pieces
    parse_command(command, commandT, name, message, clients);

    // handle user input by command type:
    switch (commandT)
    {
        case INVALID:
        {
            print_invalid_input();
            return false;
        }


        case SEND:
        {
            if (name == clientName)
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

//            // check if current client in the group he wants to create
//            if (!(std::find(clients.begin(), clients.end(), clientName)
//                  != clients.end()))
//            {
//                print_create_group(false, false, clientName, name);
//                return false;
//            }
            vector<string> noDuplicatesClients;
            noDuplicatesClients.push_back(string(clientName));

            // checks the client names are valid
            for(string& client : clients)
            {
                if(!all_alphanumeric(client))
                {
                    print_create_group(false, false, clientName, name);
                    return false;
                }
                if (!(std::find(noDuplicatesClients.begin(), noDuplicatesClients.end(),
                                client) != noDuplicatesClients.end()))
                {
                    noDuplicatesClients.push_back(client);
                }
            }

//            if(noDuplicatesClients.size() < 2)
//            {
//                print_create_group(false, false, clientName, name);
//                return false;
//            }
            break;
        }

        case WHO:
        {
            break;
        }

        case EXIT:
        {
            // TODO : check valid input?

            break;
        }
//
//            default:
//                break;
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
//    FD_ZERO(&clientListenFds);
    FD_SET(STDIN_FILENO, &readFds);
    FD_SET(clientFd, &readFds);

//    print_connection();

    while(true)
    {
        char readBuffer[MAX_BUFFER_SIZE] = {0};
        string command;
        clientListenFds = readFds;
        auto listen = (int)select(SOCKETS_NUM , &clientListenFds, NULL, NULL, NULL);

        if(listen < 0)
        {
            //error
            exit(1);
        }

        // got command from client
        if (FD_ISSET(STDIN_FILENO, &clientListenFds))
        {
            cout << "in stdin handle" << endl;
            auto inRead = (int)read(STDERR_FILENO, readBuffer,
                                                     MAX_BUFFER_SIZE -1);
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
            write(clientFd, command.c_str(), strlen(command.c_str()) + 1);

            if(command == EXIT_STR)
            {
                print_exit(false, clientName);
                exit(0);
            }

        }
        // got message from server
        else if (FD_ISSET(clientFd, &clientListenFds))
        {
            cout << "in server socket handle" << endl;

            // TODO -------------------------- :
            // Read response from server
            char buf[MAX_BUFFER_SIZE] = {0};
            auto bytesRead = (int)read(clientFd, &buf, MAX_BUFFER_SIZE - 1);
            if(bytesRead < 0)
            {
                //TODO error handling
                exit(1);
            }
            buf[bytesRead] = '\0';
            string serverResponse(buf);
            cout << "server response size: " << serverResponse.size() << endl;
//            if(serverResponse == CLOSE_SOCKET_MSG) // TODO - reachable??
//            {
//                print_exit(false, clientName); // TODO: what msg?
//                exit(1);
//            }


            tcflush(clientFd, TCIOFLUSH);
            cout << serverResponse << endl;
        }





    }
    return 0;
}