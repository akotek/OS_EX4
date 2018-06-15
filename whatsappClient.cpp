//
// Created by yoavabadi on 6/15/18.
//


#define VALID_ARGC 4
#define INVALID_ARGC_MSG "error: invalid number of arguments"
#define INVALID_CLIENT_NAME "error: invalid client name"


#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <algorithm> // for std::all_of
#include <cctype>    // for std::isalnum
//
using namespace std;

bool all_alphanumeric(const std::string& s)
{
    return std::all_of(s.begin(),
                       s.end(),
                       [](char c) { return std::isalnum(c); });
}

int main(int argc, char* argv[])
{

    // Start input validation && assignment TODO: put in helper function
    if(argc != VALID_ARGC)
    {
        cerr << INVALID_ARGC_MSG << endl;
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
    printf("Connected Successfully.\n");

    while(true)
    {
        string msg = "";
        cin >> msg;

        if (msg == "quit()")
        {
            break;
        }
        const char *cstr_msg = msg.c_str();
        send(socket_desc , cstr_msg , strlen(hello) , 0 );
        valread = read( socket_desc , buffer, 1024);
        printf("%s\n",buffer );
    }
    return 0;
}