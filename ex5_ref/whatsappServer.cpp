// ====================================================================
//                       General Description
// ====================================================================

/**
 * @brief This file contains an implementation of a WhatsappServer that handles
 * the following commands:
 * 1. who - returns to client the names of the connected clients.
 * 2. send <client_name/group_name> - Send a message to a connected client or
 *    group.
 * 3. create_group <group_name> client0,client1.. - Create a group.
 * 4. exit - Disconnect from server.
 */

// ====================================================================
//                             Includes
// ====================================================================

#include <iostream>
#include <stdlib.h>
#include <sys/select.h>
#include <regex>
#include <zconf.h>
#include <netinet/in.h>
#include <vector>
#include <netdb.h>
#include <termio.h>
using namespace std;

// ====================================================================
//                             Defines
// ====================================================================
#define LEGAL_ARGS_NUM 2
#define MAXIMAL_SOCKETS_NUM 30
#define STD_IN 0
#define BUFFER_SIZE 1500

// ====================================================================
//                             Constants
// ====================================================================
/**
 * @brief Server usage message
 */
static const string USAGE_ERROR_MSG = "Usage: whatappServer portNUm";

/**
 * @brief Server exit message
 */
static const string EXIT_MSG = "EXIT command is typed: server is shutting down";

/**
 * @brief Server exit command
 */
static const string EXIT_COMMAND = "EXIT\n";

/**
 * @brief Who command
 */
static const char *const WHO_COMMAND = "who\n";

/**
 * @brief Create Group command.
 */
static const char *const CREATE_GROUP_COMMAND = "create_group";

/**
 * @brief Client exit command
 */
static const char *const EXIT_COMMAND_CLIENT = "exit\n";

/**
 * @brief Send command
 */
static const char *const SEND_MESSAGE_COMMAND = "send";

/**
 * @brief Create group clients delimeter
 */
static const char GROUP_CREATE_DELIM = ',';

/**
 * @brief request delimter
 */
static const char INPUT_MSG_DELIM = ' ';

/**
 * @brief Unregister Client message
 */
static const char *const UNREGISTERED_CLIENT_MSG = "Unregistered successfully.";

/**
 * @brief Unregister Server message
 */
static const char *const UNREGISTERED_SEREVER_MSG = ": Unregistered "
        "successfully.";

/**
 * @brief Group min size.
 */
static const int GROUP_MIN_SIZE = 2;

/**
 * @brief Accept syscall name
 */
static const char *const ACCEPT = "accept";

/**
 * @brief Select syscall name
 */
static const char *const SELECT = "select";

/**
 * @brief Port regex format.
 */
static const char *const PORT_REGEX = "[0-9]{1,6}";

/**
 * @brief Gethostname syscall name.
 */
static const char *const GET_HOST_NAME = "gethostname";

/**
 * @brief Socket syscall name.
 */
static const char *const SOCKET = "socket";

/**
 * @brief Bind syscall name.
 */
static const char *const BIND = "bind";

/**
 * @brief Listen syscall name.
 */
static const char *const LISTEN = "listen";

/**
 * @brief Send command , client message.
 */
static const char *const WHO_SERVER_MSG = ": Requests the currently connected "
        "client names.";

/**
 * @brief Who command, client message delimeter
 */
static const char *const WHO_SERVER_DELIM = ",";

/**
 * @brief Faild to send, error messafe
 */
static const char *const SEND_ERR_MSG = "ERROR: failed to send.";

/**
 * @brief Write syscall name.
 */
static const char *const WRITE = "write";

/**
 * @brief Error message Server side.
 */
static const char *const SEND_ERR_MSG_SERVER = ": ERROR: failed to send ";

/**
 * @brief Success message, Client side.
 */
static const char *const SUCCESS_MSG_CLIENT = "Sent successfully.";

/**
 * @brief Create Group error message, Server side.
 */
static const char *const CREATE_GROUP_ERR_SERVER_MSG = ": ERROR: failed to "
        "create group \"";

/**
 * @brief Create Group error message, Client side.
 */
static const char *const CREATE_GROUP_ERR_MSG_CLIENT = "ERROR: failed to "
        "create group: \"";

/**
 * @brief Faild to connect message.
 */
static const char *const CONNECT_ERR_SERVER_MSG = " failed to connect.";

/**
 * @brief Connect syscal name.
 */
static const char *const USER_CONNECTED_SERVER_MSG = " connected.";

/**
 * @brief Close syscall name.
 */
static const char *const CLOSE = "close";

/**
 * @brief Falid to connect the server message.
 */
static const char *const FAILD_TO_CONNECT_MSG = "Failed to connect the server.";

/**
 * @brief Read syscall name.
 */
static const char *const READ = "read";


/**
 * @brief Successful connection message.
 */
static const char *const CONNECTED_SUCCESSFULLY = "Connected Successfully.";

/**
 * @brief Used client name message.
 */
static const char *const CLIENT_NAME_USED = "Client name is already in use.";

/**
 * @brief Shut down server message.
 */
static const char *const SHUTDOWN_CLIENT_MESSAGE = "shutdown";

/**
 * @brief Regex for send command.
 */
std::regex sendReg("^send ([A-Za-z0-9]+) (.*)$");
// ====================================================================
//                            Globals
// ====================================================================

map<int, string> fdToHost; // A map of connected user, fd to name.
map<string, int> hostToFd; // A map of connected user, name to fd.
map<string, vector<string>> groups; // vector of server groups.
int serverSocketFd; // Server socket
fd_set clientsFds; // A sed of clients fds.
int connectedHosts = 0; // number of connected hosts.

// ====================================================================
//                            Functions
// ====================================================================

/**
 * @brief The function prints error message to stderr.
 * @param name - function error name.
 * @param errNum - error num
 */
void printError(string name, int errNum)
{
    cerr << "ERROR: " << name << " "<< errNum << endl << flush;
}

/**
 * @brief The function checks sysall error and print error msg if error occure.
 * @param name syscall name.
 * @param retval syscall return value.
 */
void syscallCheck(string name, int retval)
{
    if (retval < 0)
    {
        printError(name, errno);
        exit(1);
    }
}

/**
 * @brief The function splits the given string according to the given
 * delimeter and returns the splited data as a vector.
 * @param s - A string to split
 * @param delim - The delimeter
 * @param result - A vector which contains the splited string.
 */
template<typename Out>
void split(const std::string &s, char delim, Out result)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    // run over the string, split accordance to given delim  and fill the
    // output vector with the data.
    while (std::getline(ss, item, delim))

    {
        *(result++) = item;
    }
}

/**
 * @brief split wrapper.
 * @param s - A string to split.
 * @param delim - The delimeter.
 * @return - A vector which contains the splited string.
 */
std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

/**
 * @brief The function establish the server that listen to given port.
 * @param port - A port to listen for incoming data.
 */
void establishServer(int port)
{
    struct sockaddr_in socketAdd;
    char hostName[BUFFER_SIZE];
    struct hostent *hostPort;
    bzero((char *) &socketAdd, sizeof(socketAdd));
    gethostname(hostName, BUFFER_SIZE);
    hostPort = gethostbyname(hostName); // set host name
    if (hostPort == NULL)
    {
        printError(GET_HOST_NAME, errno);
        exit(1);
    }
    socketAdd.sin_family = (sa_family_t) hostPort->h_addrtype;
    socketAdd.sin_port = htons(port);
    socketAdd.sin_addr.s_addr = INADDR_ANY;

    // create socket
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    syscallCheck(SOCKET, serverSocketFd);

    // bind socket
    syscallCheck(BIND, bind(serverSocketFd, (struct sockaddr *) &socketAdd,
                            sizeof(socketAdd)));

    // start listen
    syscallCheck(LISTEN, listen(serverSocketFd, MAXIMAL_SOCKETS_NUM));
}

/**
 * @brief The function create a response for who request.
 * @param fd - The client fd which request "who".
 */
void responseWho(int fd)
{
    char clientsNames[BUFFER_SIZE]; // Get client name.
    string tmp;
    auto lastClient = fdToHost.end();
    lastClient--;
    // Generate a list of connected clients
    for (auto it=fdToHost.begin(); it != fdToHost.end(); ++it)
    {
        tmp += it->second;
        if (it != lastClient)
        {
            tmp += WHO_SERVER_DELIM;
        }
    }
    strcpy(clientsNames, tmp.c_str());
    // Send response to client
    syscallCheck(WRITE, write(fd, clientsNames, strlen(clientsNames)));
    // Print to stdout server "who" msg.
    cout << fdToHost[fd] << WHO_SERVER_MSG << endl << flush;
}

/**
 * @brief The function validates that destName != clientName and that clientName
 * not already exists as a name of a client or a  group.
 * @param destName - A name of a group or other client.
 * @param clientName - The name of the client who send the request
 * @return true if destName and clientName are valide.
 */
bool validateSendReceiver(string destName, string clientName)
{
    return (groups.end() != groups.find(destName) ||
            hostToFd.end() != hostToFd.find(destName)) &&
            clientName.compare(destName);
}

/**
 * @brief The function valides that the groupName is not occupied by other
 * group or client.
 * @param groupName - A name of new group.
 * @return - True if the name isn't occupied, otherwise false.
 */
bool validateCreateGroup(string groupName)
{
    return (groups.end() == groups.find(groupName) &&
            hostToFd.end() == hostToFd.find(groupName));
}

/**
 * @brief The function sends the given message from sender to given clients.
 * @param msg - Message from sender
 * @param clients - A vector of recevier clients.
 * @param sender - The name of the sender.
 */
void sendMsgToGroup(string msg, vector<string> clients, string sender)
{
    char buf[BUFFER_SIZE];
    // Create the message of server format
    string connectedMsg = sender + ": " + msg;
    strncpy(buf, connectedMsg.c_str(), BUFFER_SIZE);
    for (unsigned int i = 0; i < clients.size(); ++i) // send message to clients
    {
        // if the current client is the server, continue.
        if (!clients[i].compare(sender))
        {
            continue;
        }
        syscallCheck(WRITE, (int)write(hostToFd[clients[i]], buf, strlen(buf)));
    }
}

/**
 * @brief The function generates a response to "Send" request.
 * @param request - Client "send" request.
 * @param clientName - Client name.
 */
void responseSend(string request, string clientName)
{
    string msg = "";
    string receiverName = "";
    unsigned long receiverIndex = 1;
    unsigned long msgIndex = 2;
    // get message and receiver name
    string in = request;
    smatch match;
    regex_search(in, match, sendReg);
    receiverName = match.str(receiverIndex);
    msg = match.str(msgIndex);

    // validate client and receiver names.
    if(!validateSendReceiver(receiverName, clientName))
    {
        cerr << clientName << SEND_ERR_MSG_SERVER << "\"" << msg << "\" to " <<
             receiverName << "." << endl << flush;
        syscallCheck(WRITE,(int) write(hostToFd[clientName],
                                       SEND_ERR_MSG, strlen(SEND_ERR_MSG)));
        return;
    }
    // Check if the receiver is a group
    auto receiver = groups.find(receiverName);
    if (receiver != groups.end()) // send message to group.
    {
        // check if client is in the group
        bool isSenderInGroup = false;
        for (unsigned int i = 0; i < receiver->second.size(); ++i)
        {
            if (!receiver->second[i].compare(clientName)) // found client!
            {
                isSenderInGroup = true;
                break;
            }
        }

        if (!isSenderInGroup) // client is not in the group, send an error.
        {
            cerr << clientName << SEND_ERR_MSG_SERVER << msg << " to " <<
                 receiverName << endl << flush;
            syscallCheck(WRITE,(int) write(hostToFd[clientName],
                                           SEND_ERR_MSG, strlen(SEND_ERR_MSG)));
            return;
        }
        // client is in the group, send message to group.
        sendMsgToGroup(msg,receiver->second, clientName);
    }
    else // send the message to other client in the server.
    {
        char dupMsg[BUFFER_SIZE] = {0};
        string tmp = clientName + ": " + msg;
        strncpy(dupMsg, tmp.c_str(), BUFFER_SIZE);
        syscallCheck(WRITE,(int) write(hostToFd[receiverName], dupMsg,
                                       strlen(dupMsg)));
        syscallCheck(WRITE, (int) write(hostToFd[clientName],
                                        SUCCESS_MSG_CLIENT,
                                        strlen(SUCCESS_MSG_CLIENT)));
    }
    cout << clientName << ": \"" << msg << "\" was sent successfully to "
         << receiverName << "." << endl;
}

/**
 * @brief The function checks that all the clients are connected to the server.
 * @param groupName - The name of the clients group
 * @param clients - A vector of potentional clients.
 * @param clientName - The name of the clients that request to create a new
 * group.
 * @return - True if all the clients are connected to the server. otherweise
 * false.
 */
bool validateExistenceOfClients(string groupName, vector<string> clients,
                              string clientName)
{
    char msg[BUFFER_SIZE];
    // walk through clients
    for(auto it1 = clients.begin(); it1 != clients.end(); ++it1)
    {
        bool connected = false;
        // check that client is connected to the server
        for(auto it2 = fdToHost.begin(); it2 != fdToHost.end() ; ++it2)
        {
            if(!(*it1).compare(it2->second))
            {
                connected = true;
            }
        }
        // If client is not connected, print and send error message.
        if(!connected)
        {
            cerr << clientName << CREATE_GROUP_ERR_SERVER_MSG
                 << groupName << "\"." << endl << flush;
            string tmp = CREATE_GROUP_ERR_MSG_CLIENT + groupName + "\".";
            strcpy(msg, tmp.c_str());
            syscallCheck(WRITE,
                         (int) write(hostToFd[clientName], msg, strlen(msg)));
            return false;
        }
    }
    return true;
}

/**
 * @brief The function chomps '\n' from the given string.
 * @param s - A string
 */
void chomp( string &s)
{
    unsigned long pos;
    if((pos = s.find('\n')) != string::npos)
        s.erase(pos);
}

/**
 * @brief The function generate a response to "create_group"
 * @param groupName - new group name.
 * @param clients - A vector of potential clients.
 * @param clientName - The client name who send the request.
 */
void responseCreateGroup(string groupName, vector<string> clients,
                         string clientName)
{
    char msg[BUFFER_SIZE];
    // validate group name and that the number of clients is legal.
    if(!validateCreateGroup(groupName) || clients.size() < GROUP_MIN_SIZE)
    {
        cerr << clientName << CREATE_GROUP_ERR_SERVER_MSG << groupName
             << "\"." << endl << flush;
        string tmp = CREATE_GROUP_ERR_MSG_CLIENT + groupName + "\".";
        strcpy(msg, tmp.c_str());
        syscallCheck(WRITE, (int) write(hostToFd[clientName], msg,
                                        strlen(msg)));
        return;
    }

    chomp(clients[clients.size()-1]); // chomp from least member name '\n'
    // validate that all clients are connected.
    if (validateExistenceOfClients(groupName, clients, clientName))
    {
        for (unsigned int i = 0; i < clients.size() ; ++i) // create group!
        {
            groups[groupName].push_back(clients[i]);
        }

        string tmp = "Group \"" + groupName + "\" was created successfully.";
        strcpy(msg, tmp.c_str());
        syscallCheck(WRITE, (int)write(hostToFd[clientName], msg, strlen(msg)));
        cout << clientName << ": Group \"" << groupName
             << "\" was created successfully" << endl << flush;
    }
}

/**
 * @brief The function generate a response to "exit" request.
 * @param fd The fd of the client who requested to exit.
 */
void responseExit(int fd)
{
    syscallCheck(WRITE, (int) write(fd, UNREGISTERED_CLIENT_MSG,
                                    strlen(UNREGISTERED_CLIENT_MSG)));
    FD_CLR(fd, &clientsFds); // Clean client from listening fds.
    string removedName = fdToHost[fd]; // Get client name

    //Remove client from data structures.
    hostToFd.erase(fdToHost[fd]);
    fdToHost.erase(fd);
    connectedHosts--;
    // Remove client from his groups.
    for (auto it=groups.begin(); it != groups.end(); ++it)
    {
        vector<string>::iterator toDelete;
        bool found = false;
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
        {
            if(!i->compare(removedName))
            {
                toDelete = i;
                found = true;
                break;
            }
        }
        if(found)
        {
            it->second.erase(toDelete);
        }
    }
    // Print exit of client to server stdout
    cout << removedName << UNREGISTERED_SEREVER_MSG << endl << flush;
}

/**
 * @brief The function manage all response creation.
 * @param fds - fds that server listens to.
 */
void handleRequest(fd_set fds)
{
    char buf[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};
    // run over connected clients.
    for (auto it = fdToHost.begin(); it != fdToHost.end() ; ++it)
    {
        if (FD_ISSET(it->first, &fds)) // if the fd is set, handle fd request.
        {
            // read the request
            syscallCheck(READ, (int) read(it->first, buf, BUFFER_SIZE - 1));
            tcflush(it->first, TCIOFLUSH);
            string temp = string (buf);
            if (!temp.compare(WHO_COMMAND)) // handle "who" request.
            {
                responseWho(it->first);
                break;
            }

            if (!temp.compare(EXIT_COMMAND_CLIENT)) // handle "exit" request.
            {
                responseExit(it->first);
                break;
            }

            sscanf(buf, "%s", command);
            vector<string> msg = split(buf, INPUT_MSG_DELIM);
            // handle "create_group" request.
            if (!string(command).compare(CREATE_GROUP_COMMAND))
            {
                responseCreateGroup(msg[1], split(msg[2], GROUP_CREATE_DELIM)
                        ,fdToHost[it->first]);
                break;
            }
            // handle "send" request.
            if (!string(command).compare(SEND_MESSAGE_COMMAND))
            {
                responseSend(buf, fdToHost[it->first]);
                break;
            }
        }
    }
}

/**
 * @brief The funcion checks if the given clientName is not occupid by a group
 * or another client.
 * @param clientName - Client name to check.
 * @return True if the name is avaliable, false otherwise.
 */
bool clientNameCheck(string clientName)
{
    // check if client name is a group name.
    for(auto it = groups.begin(); it != groups.end(); ++it)
    {
        if(!it->first.compare(clientName))
        {
            return false;
        }
    }

    // check if client name is already exists in clients.
    for(auto it = hostToFd.begin(); it != hostToFd.end(); ++it)
    {
        if(!it->first.compare(clientName))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief The function adds a new client to the server
 * @param connection Client fd.
 * @return True if the client was added successfully, otherwise false.
 */
bool addClient(int connection, bool &clientNameUsed)
{
    char name[BUFFER_SIZE] = {0};
    // read the first message from the client (accordance to the protocol the
    // clients most send his name at the first message).
    syscallCheck(READ, (int) read(connection, name, BUFFER_SIZE - 1));
    tcflush(connection,TCIOFLUSH);

    // if client name is occupied or server is full
    if (connectedHosts == MAXIMAL_SOCKETS_NUM || !(clientNameCheck(string(name))))
    {
        clientNameUsed = true;
        cerr << name << CONNECT_ERR_SERVER_MSG << endl << flush;
        return false;
    }
    // add client to server and print success message.
    fdToHost[connection] = string(name);
    hostToFd[name] = connection;
    FD_SET(connection, &clientsFds);
    cout << name << USER_CONNECTED_SERVER_MSG << endl << flush;
    connectedHosts += 1;
    return true;
}

/**
 * @brief The function reseves a new connection on server socket.
 * @param socketFd - Server socker fd
 * @return The fd of the new connection.
 */
int getConnections(int socketFd)
{
    struct sockaddr socketAdd;
    int sizeOfSocketAdd, acceptRetVal;

    sizeOfSocketAdd = sizeof(socketAdd);
    // accept new connection.
    if ((acceptRetVal = accept(socketFd, &socketAdd,
                               (socklen_t *) &sizeOfSocketAdd)) < 0)
    {
        printError(ACCEPT, errno);
        exit(1);
    }
    return acceptRetVal;
}

/**
 * @brief The function returns the highest fd that is connected to the server.
 * @return The highest fd that is connected to the server.
 */
int getHighestFd()
{
    int highestKey = 0;
    if (fdToHost.size() > 0)
    {
        auto temp = fdToHost.end();
        temp --;
        highestKey = temp->first;
    }
    return max(serverSocketFd, highestKey);
}

/**
 * @brief The function shuts down the server.
 */
void shutdown()
{
    // Send to all client exit message
    for(auto it=fdToHost.begin(); it != fdToHost.end(); ++it)
    {
        syscallCheck(WRITE, (int)write(it->first, SHUTDOWN_CLIENT_MESSAGE,
                                       strlen(SHUTDOWN_CLIENT_MESSAGE)));
    }
    cout << EXIT_MSG << endl << flush; // Print disconnect message and exit.
    exit(0);
}

/**
 * @brief This function runs and manage what'sup server.
 * @param argc - The Number of given argumens.
 * @param argv - Given argument (most be only port number)
 * @return
 */
int main(int argc, char** argv)
{
    if (argc != LEGAL_ARGS_NUM) // validate number of arguments.
    {
        cerr << USAGE_ERROR_MSG << endl << flush;
        exit(1);
    }

    // validate that the given argument is a port number.
    if (!regex_match(string(argv[1]), regex(PORT_REGEX)))
    {
        cerr << USAGE_ERROR_MSG << endl << flush;
        exit(1);
    }

    // initialize the server socket
    establishServer(atoi(argv[1]));

    // initialize fds set with std_in and server socket fd.
    FD_ZERO(&clientsFds);
    FD_SET(STD_IN, &clientsFds);
    FD_SET(serverSocketFd, &clientsFds);
    fd_set readFds;
    while (true)
    {
        readFds = clientsFds;
        // listen to fds.
        syscallCheck(SELECT,
                     select(getHighestFd() + 1, &readFds, NULL, NULL, NULL));
        char buf[BUFFER_SIZE] = {0};
        if (FD_ISSET(STD_IN, &readFds)) // got input in std in.
        {
            // read input.
            syscallCheck(READ, (int) read(STD_IN, buf, BUFFER_SIZE - 1));
            // if std in input is "EXIT", shutdown the server.
            if (!string(buf).compare(EXIT_COMMAND))

            {
                shutdown();
                syscallCheck(CLOSE, close(serverSocketFd));
            }
        }
        // if server socket is set, get the connection and add a new client.
        else if (FD_ISSET(serverSocketFd, &readFds))
        {
            int connection = getConnections(serverSocketFd);
            syscallCheck(ACCEPT, connection);
            bool clientNameUsedError = false;
            if(!addClient(connection, clientNameUsedError))
            {
                if(!clientNameUsedError)
                {
                    syscallCheck(WRITE, (int)write(connection,
                                               FAILD_TO_CONNECT_MSG,
                                               strlen(FAILD_TO_CONNECT_MSG)));
                }
                else
                {
                    syscallCheck(WRITE, (int)write(connection, CLIENT_NAME_USED,
                                                   strlen(CLIENT_NAME_USED)));
                }
            }
            else{
                syscallCheck(WRITE, (int)write(connection, CONNECTED_SUCCESSFULLY,
                                               strlen(CONNECTED_SUCCESSFULLY)));
            }
        }
        else // At least one of the clients is set, handle client request.
        {
            handleRequest(readFds);
        }
    }
}