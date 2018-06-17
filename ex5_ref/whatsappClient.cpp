// ====================================================================
//                       General Description
// ====================================================================

/**
 * @brief This file contains an implementation of a WhatsappClient which
 * represenes a client which conndects to the server and responds to the
 * following commands:
 * 1. who - asks the server for the names of the connected clients.
 * 2. send <client_name/group_name> - asks the server to send a message to a
 * connected client or group.
 * 3. create_group <group_name> client0,client1.. - asks the server to reate a
 * group with the given name.
 * 4. exit - asks the sever to disconnect this specific client from the server.
 *
 * The client also validates the input it recieves and respons accordingly
 * whether it's an invalid command/input and prints out the server's response
 * to a certain request or a message from the server (another clients message
 * for instance).
 */

// ====================================================================
//                             Includes
// ====================================================================

#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <zconf.h>
#include <regex>
#include <algorithm>
#include <termio.h>
#include <regex>

// ====================================================================
//                             Defines
// ====================================================================

#define ARG_NUM 4
#define MAX_MSG_LEN 1500
#define SEND_GROUP_INDEX 2
#define EXIT_WHO_INDEX 4
#define STD_IN 0
#define MAXIMAL_SOCKETS_NUM 2


using namespace std;

// ====================================================================
//                             Constants
// ====================================================================

/* An enum for all the command types */
enum COMMAND_TYPE{SEND_COMMAND, CREATE_GROUP_COMMAND, EXIT_OR_WHO_COMMAND};
/**
 * @brief Client usage message
 */
const string USAGE_MESSAGE = "Usage: whatsappClient clientName serverAddress "
        "serverPort";
/**
 * @brief Client failing to connect to server message.
 */
const string FAILED_CONNECT_MESSAGE = "Failed to connect the server.";
/**
 * @brief Client unregistered message.
 */
const string UNREGISTERED_MESSAGE = "Unregistered successfully.";
/**
 * @brief Client invalid input message.
 */
const string INVALID_INPUT_MESSAGE = "ERROR: Invalid input.";
/**
 * @brief Client beggining of read function error.
 */
const string ERROR_READ_MESSAGE = "ERROR: read ";
/**
 * @brief Client beggining of write function error.
 */
const string ERROR_WRITE_MESSAGE = "ERROR: write ";
/**
 * @brief Client failing to create group message.
 */
const string CREATE_GROUP_ERROR_MESSAGE = "ERROR: failed to create group: \"";
/**
 * @brief Client failing to send message.
 */
const string SEND_ERROR_MESSAGE = "ERROR: failed to send.\n";
/**
 * @brief send command.
 */
const string SEND = "send";
/**
 * @brief exit command.
 */
const string EXIT = "exit";
/**
 * @brief who command.
 */
const string WHO = "who";
/**
 * @brief create_group command.
 */
const string CREATE_GROUP = "create_group";
/**
 * @brief char constant for a single space.
 */
const char SPACE = ' ';
/**
 * @brief string constant for a single space (used as separator).
 */
const string SPACE_SEP = " ";
/**
 * @brief tring constant for a single comma (used as separator).
 */
const string COMMA = ",";

/**
 * @brief Shut down server message.
 */
static const char *const SHUTDOWN_CLIENT_MESSAGE = "shutdown";

/**
 * @brief Used client name message.
 */
static const char *const CLIENT_NAME_USED = "Client name is already in use.";

static const char *const WHO_ERROR_MSG = "ERROR: failed to receive list of connected clients.";
/**
 * @brief Regex for recognizing a string that's made out of numbers only.
 */
std::regex numbersOnlyReg("[0-9]+");

/**
 * @brief Regex for recognizing a string that's a valid ip.
 */
std::regex ipAddReg("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");

/**
 * @brief Regex for recognizing a string that's made out of numbers and
 * letters only.
 */
std::regex lettersAndNumReg("^[A-Za-z0-9]+$");

/**
 * @brief Regex for separating the actual message from the line break token.
 */
std::regex noLineBreak("(.*)(?:\\r?\\n.*)*");

/**
 * @brief Regex for recognizing a legal structure for group participants.
 */
std::regex participantsReg("^([A-Za-z0-9]+|([A-Za-z0-9]+,)+([A-Za-z0-9]+))$");


// ====================================================================
//                            Globals
// ====================================================================

char *serverAddress; // the server's address
char *clientName; // client's name
unsigned short serverPort; // the server's port number
struct sockaddr_in sa; // sockaddr_in struct for socket address
struct hostent *hp; // hostent struct for host port
int socketFd; // this client's socket file descriptor
fd_set readFds; // fd_set of of this client's fd and standard input

// ====================================================================
//                            Functions
// ====================================================================

/**
 * The function checks sysall error and print error msg if error occure.
 * @param name syscall name.
 * @param retval syscall return value.
 */
void syscallCheck(string name, int retval)
{
    if (retval < 0)
    {
        cerr << "ERROR: " << name << " " << errno << endl << flush;
        exit(1);
    }
}

/**
 * Validates the program's arguments. Exits if input is invalid.
 * @param argc Amount of parameters the program has recieved.
 * @param argv array of char* parameters.
 */
void checkParams(int argc, char* argv[])
{
    char *client = argv[1];
    char *server = argv[2];
    char *port = argv[3];

    if (argc != ARG_NUM)
    {
        cerr << USAGE_MESSAGE << endl << flush;
        exit(1);
    }
    if(!regex_match(port, numbersOnlyReg) || !regex_match(server, ipAddReg)
       || !regex_match(client, lettersAndNumReg))
    {
        cerr << FAILED_CONNECT_MESSAGE << endl << flush;
        exit(1);
    }
    // set global variables if input is indeed valid
    clientName = client;
    serverAddress = server;
    serverPort = atoi(port);
}

/**
 * Generates a socket. Connects to the server according to the given program
 * arguments.
 */
void createSocket()
{
    if ((hp = gethostbyname(serverAddress)) == NULL) // get host port
    {
        cerr << FAILED_CONNECT_MESSAGE << endl << flush;
        exit(1);
    }
    memset(&sa, 0, sizeof(sa));
    memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)serverPort);
    // create the socket
    if ((socketFd = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
    {
        cerr << FAILED_CONNECT_MESSAGE << endl << flush;
        exit(1);
    }
    // connect to the server
    if (connect(socketFd,(struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        close(socketFd);
        cerr << FAILED_CONNECT_MESSAGE << endl << flush;
        exit(1);
    }
    // write the client name to the server
    if (write(socketFd, clientName, strlen(clientName)) < 0)
    {
        cerr << ERROR_WRITE_MESSAGE << errno << "." << endl << flush;
        exit(1);
    }
}

/**
 * @brief The function splits the given string according to the given
 * delimeter and returns the splited data as a vector.
 * @param s - A string to split.
 * @param delim - The delimeter.
 * @param result - A vector which contains the splited string.
 */
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
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
 * Validates a send command.
 * @param parameters string vector which is generated by splitting the send
 * command according to space delimiter.
 * @param input original user input.
 * @param editedInput final output which has to be sent to server - if the
 * input is indeed valid.
 * @return true if the given send command is valid, false otherwise.
 */
bool checkSendCommand(vector<string> parameters, string input,
                      string &editedInput)
{
    unsigned int minParamSize = 3; // minimal amount of valid parameters size
    if(parameters.size() < minParamSize)
    {
        return false;
    }
    // make sure recipient name is only made out of numbers and letters
    string name = parameters[1];
    if(!regex_match(name, lettersAndNumReg))
    {
        return false;
    }
    editedInput = input;
    return true;
}

/**
 * Validates a create_group command.
 * @param parameters string vector which is generated by splitting the create
 * group command according to space delimiter.
 * @param input original user input.
 * @param editedInput final output which has to be sent to server - if the
 * input is indeed valid.
 * @param groupName the group's name.
 * @return true if the given send command is valid, false otherwise.
 */
bool checkCreateGroupCommand(vector<string> parameters, string input,
                             string &editedInput, string &groupName)
{
    unsigned int minParamSize = 3; // minimal amount of valid parameters size
    if(parameters.size() < minParamSize)
    {
        return false;
    }
    // make sure group's name is only made out of numbers and letters
    groupName = parameters[1];
    if(!regex_match(groupName, lettersAndNumReg))
    {
        return false;
    }
    // generate create_group message without duplicates and with client name
    // if the client's name is missing
    char delimiter = ',';
    string participants = parameters[2];
    if(!regex_match(participants, participantsReg))
    {
        return false;
    }
    vector<string> parVec = split(participants, delimiter);
    bool clientInGroup = false; // indicator if client's name is in group
    sort(parVec.begin(), parVec.end());
    parVec.erase(std::unique(parVec.begin(), parVec.end()), parVec.end());
    for(auto it = parVec.begin(); it != parVec.end(); ++it)
    {
        if(!(*it).compare(clientName))
        {
            clientInGroup = true;
        }
    }
    // generate the edited input's outcome
    editedInput = CREATE_GROUP + SPACE_SEP + groupName + SPACE_SEP;
    if(!clientInGroup)
    {
        parVec.push_back(clientName);
    }
    for(unsigned int i = 0; i < parVec.size(); ++i)
    {
        if(i != parVec.size() - 1)
        {
            editedInput = editedInput + parVec[i] + COMMA;
        }
        else
        {
            editedInput = editedInput + parVec[i];
        }
    }
    return true;
}

/**
 * Checks if the command of the given type is in the correct format in terms
 * of spacing.
 * @param input original user input.
 * @param command The type of the command (enum defined above).
 * @return true if the given command is correct spacing, false otherwise.
 */
bool checkSpaces(string input, COMMAND_TYPE command)
{
    size_t firstSpace = input.find_first_of(" "); // index of first space
    size_t secSpace = string::npos; // index of second space
    size_t thirdSpace = string::npos; // index of third space

    // if there's a space in who/exit command - illegal
    if(firstSpace != string::npos && command == EXIT_OR_WHO_COMMAND)
    {
        return false;
    }
    if (firstSpace != string::npos)
    {
        input[firstSpace] = '*';
        secSpace = input.find_first_of(" ", firstSpace + 1);
    }
    if (secSpace != string::npos)
    {
        input[secSpace] = '*';
        thirdSpace = input.find_first_of(" ", secSpace + 1);
    }
    // if there are two or more spaces between command and first parameter in
    // send/create_group command - illegal
    if((command == CREATE_GROUP_COMMAND || command == SEND_COMMAND) &&
            (firstSpace + 1) == secSpace)
    {
        return false;
    }
    // if there's no third space in create_group command - illegal
    if(command == CREATE_GROUP_COMMAND && thirdSpace != string::npos)
    {
        return false;
    }
    return true;
}

/**
 * Validates a given input before sending it to the sever.
 * @param input original user input.
 * @param editedInput final output which has to be sent to server - if the
 * input is indeed valid.
 * @return true if the given input is valid, false otherwise.
 */
bool checkInput(char *input, string &editedInput)
{
    string in = input;
    smatch match; // smatch for regex use
    regex_search(in, match, noLineBreak);
    in = match.str(1); // remove line break from input

    vector<string> parameters = split(in, SPACE); // get command parameters
    string command = "";
    bool inputValid = false;
    if (parameters.size() > 0)
    {
        command = parameters[0]; // command name
        inputValid = true; // indicator if the command is valid
    }

    if(!command.compare(SEND)) // it's a send command
    {
        string inputSpaceCheck = in;
        if(checkSpaces(inputSpaceCheck, SEND_COMMAND))
        {
            if(!checkSendCommand(parameters, in, editedInput))
            {
                cout << SEND_ERROR_MESSAGE << flush;
                return false;
            }
        }
        else
        {
            cerr << SEND_ERROR_MESSAGE << flush;
            return false;
        }
    }
    else if(!command.compare(CREATE_GROUP)) // it's a create_group command
    {
        string inputSpaceCheck = in;
        string groupName = "";
        if (parameters.size() > 1)
        {
            groupName = parameters[1];

        }
        if(checkSpaces(inputSpaceCheck, CREATE_GROUP_COMMAND))
        {
            if(!checkCreateGroupCommand(parameters, in, editedInput, groupName))
            {
                cerr << CREATE_GROUP_ERROR_MESSAGE << groupName << "\".\n"
                     << flush;
                return false;
            }
        }
        else
        {
            cout << CREATE_GROUP_ERROR_MESSAGE << groupName << "\".\n"  <<
                                                                        flush;
            return false;
        }
    }
    // it's a who or exit command
    else if(!command.compare(WHO))
    {
        string inputSpaceCheck = in;
        if(checkSpaces(inputSpaceCheck, EXIT_OR_WHO_COMMAND))
        {
            editedInput = input;
        }
        else
        {
            cerr << WHO_ERROR_MSG << endl << flush;
            return false;
        }
        if(parameters.size() > 1)
        {
            cerr << WHO_ERROR_MSG << endl << flush;
            return false;
        }
        editedInput = input;
    }
    else if(!command.compare(EXIT))
    {
        string inputSpaceCheck = in;
        if(checkSpaces(inputSpaceCheck, EXIT_OR_WHO_COMMAND))
        {
            editedInput = input;
        }
        else
        {
            inputValid = false;
        }
        if(parameters.size() > 1)
        {
            inputValid = false;
        }
        editedInput = input;
    }
    else // if didn't match any command - it's an invalid input
    {
        cerr << INVALID_INPUT_MESSAGE << endl << flush;
        return false;
    }

    if(!inputValid)
    {
        cerr << INVALID_INPUT_MESSAGE << endl << flush;
        return false;
    }
    return true;
}

/**
 * Process an input from the sever.
 * @param bufferRead Input from the server.
 */
void processReadInput(string bufferRead)
{
    // if it's an unregistered message, or a failing to connect message - exit.
    if(!bufferRead.compare(FAILED_CONNECT_MESSAGE) ||
            !bufferRead.compare(CLIENT_NAME_USED))
    {
        cerr << bufferRead << endl << flush;
        exit(1);
    }

    if(!bufferRead.compare(SHUTDOWN_CLIENT_MESSAGE))
    {
        exit(0);
    }

    if(!bufferRead.compare(UNREGISTERED_MESSAGE))
    {
        cout << bufferRead << endl << flush;
        exit(0);
    }

    // otherwise print out the message
    cout << bufferRead << endl << flush;
}

/**
 * @brief main function of the program. Runs and manages the whatsappClient.
 * @param argc The Number of given argumens.
 * @param argv Given char * array of arguments.
 * @return 0 when program exits successfully, error and different return
 * number otherwise.
 */
int main(int argc, char* argv[])
{
    fd_set listenFds; // fds set with std_in and client socket fd.
    checkParams(argc, argv); // validate arguments
    createSocket(); // establish a connection with the server
    FD_ZERO(&readFds);
    FD_SET(STD_IN, &readFds);
    FD_SET(socketFd, &readFds);

    while(true)
    {
        char bufferRead[MAX_MSG_LEN] = {0}; // read buffer
        char bufferReadEdited[MAX_MSG_LEN] = {0}; // edited read buffer
        string editedRead = ""; // string of edited read (copied to buffer)
        listenFds = readFds;
        syscallCheck("select", select(4 , &listenFds, NULL, NULL, NULL));

        // got input into the standard input
        if (FD_ISSET(STD_IN, &listenFds))
        {
            int size = -1;
            // read input
            if ((size = read(STD_IN, bufferRead, MAX_MSG_LEN -1)) < 0)
            {
                cerr << ERROR_READ_MESSAGE << errno << ".\n";
                exit(1);
            }
            if(size && checkInput(bufferRead, editedRead))
            {
                strcpy(bufferReadEdited, editedRead.c_str());
                if (write(socketFd, bufferReadEdited,
                          strlen(bufferReadEdited) + 1) < 0)
                {
                    cerr << ERROR_WRITE_MESSAGE << errno << ".\n" << flush;
                    exit(1);
                }
            }

        }
        // got input to the client's socket
        else if(FD_ISSET(socketFd, &listenFds))
        {
            if ((read(socketFd, bufferRead, MAX_MSG_LEN)) < 0)
            {
                cerr << ERROR_READ_MESSAGE << errno << ".\n" << flush;
                exit(1);
            }
            else
            {
                tcflush(socketFd, TCIOFLUSH);
                processReadInput(bufferRead); // process the input
            }
        }
    }
}