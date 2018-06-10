#include <iostream>
#include<cstdio>
#include <netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include <cstring>

using namespace std;

int main()
{

    // Creates a socket
    // SOCK_STREAM means we use TCP
    int socket_descriptor = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_descriptor != 0) cout << "error" << endl;

    // Init server
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    server.sin_addr.s_addr = inet_addr("216.58.212.238"); // Converts ip to
    memset(&(server.sin_zero), '\0', 8);

    if (connect(socket_descriptor, (struct sockaddr *) &server, sizeof(server)
    ) <0){
        cout << "error connecting to google.com " << endl;
        puts("connect error");
        return 1;
    }


    puts("Connected");
    return 0;
}
