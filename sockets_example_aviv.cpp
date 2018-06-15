#include <iostream>
#include<cstdio>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <cstring>
#include <unistd.h>

using namespace std;

int main()
{
    // Main logic:
    // 1. Create socket
    // 2. Bind to addr && port
    // 3. Put in listening mode
    // 4. Accept connections and process requests

    // This sample is just for one socket connection
    // After launching this program- use telnet localhost {PORT}
    // And write data to client (it will return back the data)

    // Creates master socket
    // SOCK_STREAM means we use TCP
    int socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        puts("error creating socket");
        return -1;
    }
    puts ("Socket created");

    // Init client
    struct sockaddr_in client;

    // Init server -- Input module:
    struct sockaddr_in server;
    uint16_t port = 8888;
    const char* addr = {"127.0.0.1"};   // localhost

    server.sin_family = AF_INET;    // set family to internet
    server.sin_port = htons(port);    // host to network short, port is 2bytes==short
    server.sin_addr.s_addr = inet_addr(addr); // Converts ip addr to num for network
    memset(&(server.sin_zero), '\0', 8);    // put all nums to zero

    // Bind server:
    int bindSysCall = bind(socket_desc, (struct sockaddr *)&server , sizeof(server));
    if (bindSysCall < 0) {
        close(socket_desc); // close socket
        puts("error binding server");
        return -1;
    }
    puts("Binding succeded");


    // Listen server:
    int maxNumOfConnects = 3;
    listen(socket_desc, maxNumOfConnects);

    puts("Waiting for incoming connections...");
    int c = sizeof(struct sockaddr_in);
    // A new socket will be created
    // The one which is to be used by telnet (accept returns a socket)
    int client_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

    if (client_socket < 0){
        puts("accepting a connection failed");
        return -1;
    }
    puts("Connection by client to server accepted");

    char client_message[2000];
    ssize_t read_size;

    while( (int)(read_size = recv(client_socket , client_message , 2000 , 0)) > 0 )
    {
        char msg[] = "Got your input\n";
        strcat(msg, client_message);
        write(client_socket , msg , strlen(msg)); // writes data
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        return -1;
    }
    else if(read_size == -1)
    {
        puts("recv failed");
    }

    cout << "Example succeded" << endl;
    return 0;
}
