//
// Created by yoavabadi on 6/15/18.
//


#define VALID_ARGC 4
#define INVALID_ARGC_MSG "error: invalid number of arguments"
#define INVALID_COMMAND_MSG "ERROR: Invalid input.\n"
#define USAGE_MSG "Usage: whatsappClient​ clientName serverAddress serverPort"
#define INVALID_CLIENT_NAME "error: invalid client name"
#define NAME_IN_USE "NAME_IN_USE"
#define GROUP_ERROR "ERROR: failed to create group %s.\n"
#define SEND_ERROR "ERROR: failed to send.\n"
#define SEND_SUCCESS "Sent successfully. \n"

#include "whatsappio.h"
#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cctype>    // for std::isalnum
//
using namespace std;

//vector<string> groups;


bool all_alphanumeric(const std::string& s)
{
    return std::all_of(s.begin(),
                       s.end(),
                       [](char c) { return std::isalnum(c); });
}

void handleGroupValidation()
{

}






int main(int argc, char* argv[])
{

    // Start input validation && assignment TODO: put in helper function
    if(argc != VALID_ARGC)
    {
        cerr << INVALID_ARGC_MSG << endl;
        cout << USAGE_MSG << endl;
        return 1;
    }
    // validate input
    const char* clientName = argv[1];
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
    int socket_desc = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};
    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    // set socket options - optional
//    auto int on=1;
//    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on) < 0)
//        perror("setsockopt");


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
        printf("\nConnection Failed \n");
        return -1;
    }
    // TODO: name in use check


    printf("Connected Successfully.\n");




    while(true)
    {
        string command;
        getline(cin, command); // get the command from the user

        // inputs to insert to the parse function
        command_type commandT;
        std::string name;
        std::string message;
        std::vector<std::string> clients;

        // splits user input to workable pieces
        parse_command(command, commandT, name, message, clients);

        // handle user input by command type:
        switch (commandT)
        {
            case INVALID:
                printf(INVALID_COMMAND_MSG);
                continue;

            case CREATE_GROUP:
                // check if group_name is alphanumeric &
                // there is more then 1 client
                if(!all_alphanumeric(name) || clients.size() < 2)
                {
                    printf(GROUP_ERROR, name);
                    continue;
                }

                // check if current client in the group he wants to create
                if (!(std::find(clients.begin(), clients.end(), clientName)
                      != clients.end()))
                {
                    printf(GROUP_ERROR, name);
                    continue;
                }
                std::vector<std::string> noDuplicatesClients;

                // checks the client names are valid
                for(string& client : clients)
                {
                    if(!all_alphanumeric(client))
                    {
                        printf(GROUP_ERROR, name);
                        continue;
                    }
                    if (!(std::find(noDuplicatesClients.begin(), noDuplicatesClients.end(),
                                  client) != noDuplicatesClients.end()))
                    {
                        noDuplicatesClients.pop_back(client);
                    }
                }
                clients.swap(noDuplicatesClients);
                // TODO: ^ should be handled in server side ^

//                groups.pop_back(name); // add group to client group


            case SEND:
                if (name == clientName)
                {
                    printf(SEND_ERROR);
                    continue;
                }
        }




        const char *cstr_msg = msg.c_str();
        send(socket_desc , cstr_msg , strlen(msg), 0);
        valread = read( socket_desc , buffer, 1024);

        // handle server response
        printf("%s",buffer);
        if(commandT == EXIT)
        {
            exit(0);
        }

    }
    return 0;
}