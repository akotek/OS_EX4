#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
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

    // Creates a socket
    // SOCK_STREAM means we use TCP,
    // AF_INET means we use IPv4
    int socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        puts("error creating socket");
        return -1;
    }
    puts ("Socket created");
    printf("socket desc: %d \n", socket_desc);

    // Init client
    struct sockaddr_in client;

    // Init server -- Input module:
    struct sockaddr_in server;
    uint16_t port = 8888;
    const char* addr = {"127.0.0.1"};   // localhost

    server.sin_family = AF_INET;    // set family to internet
    server.sin_port = htons(port);    // host to network short, port is 2bytes==short
    auto in_addr = inet_addr(addr);
    if(in_addr == INADDR_NONE)
    {
        puts("error invalid address");
        return -1;
    }
    server.sin_addr.s_addr = in_addr; // Converts ip addr to num for network
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

    int client_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

    // TODO: select?

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


    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                                       "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr*)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    return 0;
}
