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

using namespace std;

// --------- Constants ---------
#define VALID_ARGC 4
#define EMPTY_STRING ""
static const int MAX_BUFFER_SIZE = 256;

static const std::regex portRegex("^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
static const std::regex ipRegex("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\."
                                        "[0-9]{1,3}$");

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

void establishConnection()
{

    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    memcpy((char *)&server_sockaddr.sin_addr, hp->h_addr,
           (unsigned int)hp->h_length);
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

//    // Read response from server
//    char buf[MAX_BUFFER_SIZE] = {0};
//    auto bytesRead = (int) read(clientFd, &buf, MAX_BUFFER_SIZE - 1);
//    buf[bytesRead] = '\0';
//    string serverResponse(buf);
//    // TODO -------------------------- :
//    cout << serverResponse << endl;
}



/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[])
{
    fd_set clientFds; // stdin and socket

    // Input validation && assignment
    validateArguments(argc, argv);

    // create a connection to the server
    establishConnection();

    FD_ZERO(&readFds);
    FD_SET(STDIN_FILENO, &readFds);
    FD_SET(clientFd, &readFds);

    print_connection();

    while(true)
    {
        string command;
        getline(cin, command); // get the command from the user

        // parse_command errors out with empty string
        if(command == EMPTY_STRING)
        {
            print_invalid_input();
            continue;
        }

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
                continue;
            }


            case SEND:
            {
                if (name == clientName)
                {
                    print_send(false, false, clientName, name, command);
                    continue;
                }
                break;
            }

            case CREATE_GROUP:
            {
                // check if group_name is alphanumeric &
                // there is more then 1 client
                if(!all_alphanumeric(name) || clients.size() < 2)
                {
                    print_create_group(false, false, clientName, name);
                    continue;
                }

                // check if current client in the group he wants to create
                if (!(std::find(clients.begin(), clients.end(), clientName)
                      != clients.end()))
                {
                    print_create_group(false, false, clientName, name);
                    continue;
                }
                vector<string> noDuplicatesClients;
                noDuplicatesClients.push_back(clientName);

                // checks the client names are valid
                for(string& client : clients)
                {
                    if(!all_alphanumeric(client))
                    {
                        print_create_group(false, false, clientName, name);
                        continue;
                    }
                    if (!(std::find(noDuplicatesClients.begin(), noDuplicatesClients.end(),
                                    client) != noDuplicatesClients.end()))
                    {
                        noDuplicatesClients.push_back(client);
                    }
                }

                if(noDuplicatesClients.size() < 2)
                {
                    print_create_group(false, false, clientName, name);
                    continue;
                }
                break;
            }

            case WHO:
            {
                break;
            }

            case EXIT:
            {
                break;
            }
//
//            default:
//                break;
        }

        char buffer[1024] = {0};
        const char *cstr_msg = command.c_str();
        send(clientFd , cstr_msg , strlen(cstr_msg), 0);
        valread = read(clientFd , buffer, 1024);
        // handle server response
        // if server exit before client
        string str(buffer);
        if(str == EMPTY_STRING)
        {
            exit(1); // TODO: error msg?
        }
        printf("%s",buffer);
        if(commandT == EXIT)
        {
            exit(0);
        }

    }
    return 0;
}