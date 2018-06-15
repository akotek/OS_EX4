#include <iostream>
#include<cstdio>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>
using namespace std;

#define MAX_CLIENTS 50

void initSockets(int &socketList[]){
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        socketList[i] = 0;
    }
}

int main()
{

    int masterSocket;   // Master socket
    fd_set readfds; // Socket descriptors
    int clientSockets[MAX_CLIENTS];  // maxClient num is 50
    // Init all sockets to zero == means we read from terminal (STDIN)
    initSockets(clientSockets);


    // Creates master socket
    // SOCK_STREAM means we use TCP
    masterSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (masterSocket == -1) {
        puts("error creating socket");
        exit(-1);
    }
    puts ("Socket created");

    // Init and bind Server
    struct sockaddr_in server;
    uint16_t port = 8888;
    const char* addr = {"127.0.0.1"};   // localhost
    server.sin_family = AF_INET;    // set family to internet
    server.sin_port = htons(port);    // host to network short, port is 2bytes==short
    server.sin_addr.s_addr = inet_addr(addr); // Converts ip addr to num for network
    memset(&(server.sin_zero), '\0', 8);    // put all nums to zero
    int bindSysCall = bind(masterSocket, (struct sockaddr *)&server , sizeof(server));
    if (bindSysCall < 0) {
        close(masterSocket); // close socket
        puts("error binding server");
        return -1;
    }
    puts("Binding succeded");


    // Listen server:
    int maxNumOfConnects = 3;
    listen(masterSocket, maxNumOfConnects);

    int serverlen = sizeof(server);
    puts("Waiting for incoming connections...");

    int max_sd, socket_desc, activity;
    const char *msg = {"Hellooooo"};
    while(true){
        // Select blocks the calling process untill there is an
        // Activity on any of the sets of FD or till timeout
        // select Returns number of ready socket FD's

        // Clear socketSet
        FD_ZERO(&readfds);
        // Add MasterSocket to set
        FD_SET(masterSocket, &readfds);
        max_sd = masterSocket;

        // Add all sockets to set:
        for (int i =0; i< MAX_CLIENTS; i++){
            socket_desc = clientSockets[i];

            // if socket is valid (working one):
            // add it to fileDescriptor set
            if (socket_desc > 0){
                FD_SET(socket_desc, &readfds);
            }

            if (socket_desc > max_sd)
            {
                // We hold Highest FD number
                // for select function
                max_sd = socket_desc;
            }
        }
        // Wait for an activity on one of the sockets, timeout is NULL,
        activity = select(max_sd+1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0){
            puts("select error");
        }
        // ISSET returns none-zero if bit
        int fdSetSysCall = FD_ISSET(masterSocket, &readfds);
        // If something happened on the master socket ,
        // then its an incoming connection
        if (fdSetSysCall != 0){
            // ConnectNewClient();

            int newSocket = accept(masterSocket, (struct sockaddr *)&server,
                                   (socklen_t*)&serverlen);
            if (newSocket < 0){
                puts("accept error");
                exit(-1);
            }
            // inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d"
                           " \n", newSocket , inet_ntoa(server.sin_addr) ,
                   ntohs(server.sin_port));

            // Send new connection msg:
            ssize_t sendSysCall = send(newSocket, msg, strlen(msg), 0);
            if (sendSysCall != strlen(msg)){
                puts("error with sending");
            }
            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                //if position is empty
                if( clientSockets[i] == 0 )
                {
                    clientSockets[i] = newSocket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        } // end if
        //else its some IO operation on some other socket
        //todo FIN THIS

        // HandleClientRequest()


    }


}