//
// Created by yoavabadi on 6/15/18.
//


#define VALID_ARGC 4
#define INVALID_ARGC_MSG "error: invalid number of arguments"
#define INVALID_CLIENT_NAME "error: invalid client name"
#define INVALID_COMMAND_MSG "ERROR: Invalid input.\n"
#define EMPTY_STRING ""

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
//
using namespace std;


bool all_alphanumeric(const std::string& s)
{
    return all_of(s.begin(),s.end(),
                       [](char c) { return isalnum(c); });
}

//void handleGroupValidation()
//{
//
//}
//





int main(int argc, char* argv[])
{

    // Start input validation && assignment TODO: put in helper function
    if(argc != VALID_ARGC)
    {
        cerr << INVALID_ARGC_MSG << endl;
        return 1;

    }
    // validate input
    const string clientName = argv[1];
    const char* server_ip = argv[2];
    int server_port = atoi(argv[3]);


    // client name must be alphanumeric (letters & digits only)
    if (!all_alphanumeric(clientName))
    {
        cerr << INVALID_CLIENT_NAME << endl;
        return 1;
    }

    // End of input validation && assignment



    struct sockaddr_in address;
    struct hostnet* hp;
    int socket_desc = 0, valread;
    struct sockaddr_in serv_addr;
//
//    if((hp = gethostbyname(server_ip)) == NULL)
//    {
//        print_fail_connection();
//        exit(1);
//    }
//    memset(&address, 0, sizeof(address));
//    memcpy((char*)&address.sin_addr, hp->h_addr, hp->h_length);
//    address.sin_family = hp->h_addrtype;
//    address.sin_port= hp->htons((u_short)server_port);
//
//

    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }





    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        print_fail_connection();
        return -1;
    }



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
                printf(INVALID_COMMAND_MSG);
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
        send(socket_desc , cstr_msg , strlen(cstr_msg), 0);
        valread = read(socket_desc , buffer, 1024);
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