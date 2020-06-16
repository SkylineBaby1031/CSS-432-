//Program 4
//Name:Lu Lu
//CSS 432 Networking
//Date:May 26th, 2020
#include <cstdlib>
#include "Socket.h"

using namespace std;


const int ON = 1;                   // for asynchronous connection switch
const int ACCEPT = 5;               // number of clients to allow in queue


int main(int argc, char** argv) {
    int    counter;                         
    int    hostPort;                       
    int    serverSd;                        
    int    clientSd;                      
    struct sockaddr_in acceptSockAddr;    
    struct sockaddr_in clientAddr;         
    socklen_t addrSize = sizeof(clientAddr);
    struct hostent* clientEnt = NULL;     
    struct in_addr  tempAddr;              
    char* clientIp = NULL;     // client dotted decimal IP address
    char* currentIp = NULL;     // known dotted decimal IP address
    uint16_t        clientPort;             
    in_addr_t       clientLoc;             
    bool            trusted = false;  

    // check argument count
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " PORT" << endl;
        exit(EXIT_FAILURE);
    } 

    // read a port from argument list
    hostPort = atoi(argv[1]);
    if (hostPort < 1024 || hostPort > 65535)
    {
        cerr << argv[0] << ": port must be between 1024 and 65535" << endl;
        exit(EXIT_FAILURE);
    }

    // prepare the socket
    acceptSockAddr.sin_family = AF_INET;   // Address Family Internet
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSockAddr.sin_port = htons(hostPort);

    // active open, ensure success before continuing
    if ((serverSd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << argv[0] << ": socket failure" << endl;
        exit(EXIT_FAILURE);
    } 

    // setup server socket, bind, and listen for client connection
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char*)&ON, sizeof(int));
    bind(serverSd, (sockaddr*)&acceptSockAddr, sizeof(acceptSockAddr));
    listen(serverSd, ACCEPT);

    // sleep indefinitely
    while (true) {
        // establish client connection
        clientSd = accept(serverSd, (sockaddr*)&clientAddr, &addrSize);

        // let a child process the connection
        if (fork() == 0) {
            getpeername(clientSd, (sockaddr*)&clientAddr, &addrSize);

            // obtain client information
            clientIp = inet_ntoa(clientAddr.sin_addr);
            clientPort = ntohs(clientAddr.sin_port);
            clientLoc = inet_addr(clientIp);
            clientEnt = gethostbyaddr(&clientLoc,
                sizeof(unsigned int), AF_INET);

            // print known client information
            cout << "\nClient address = " << clientIp
                << " port = " << clientPort << endl;

            if (clientEnt != NULL) {
                cout << "Official hostname: " << clientEnt->h_name << endl;
                cout << "Aliases: " << endl;
                counter = 0;

                // check for aliases
                if (clientEnt->h_aliases[counter] == NULL) {
                    cout << "    none" << endl;
                } 
                else {
                    while (clientEnt->h_aliases[counter] != NULL) {
                        cout << "    "
                            << clientEnt->h_aliases[counter++] << endl;
                    } 
                } 
                cout << "IP addresses: " << endl;
                counter = 0;

                // check for IP addresses
                if (clientEnt->h_addr_list[counter] == NULL) {
                    cout << "    none" << endl;
                } // end if (clientEnt->h_addr_list[counter] == NULL)
                else {
                    while (clientEnt->h_addr_list[counter] != NULL) {
                        tempAddr.s_addr =
                            *(u_long*)clientEnt->h_addr_list[counter++];
                        currentIp = inet_ntoa(tempAddr);
                        cout << "    " << currentIp;

                        // check client validity
                        if (strcmp(clientIp, currentIp) == 0) {
                            cout << " ... hit!";
                            trusted = true;
                        }

                        cout << endl;
                    } 
                }
            }

            if (trusted) {
                cout << "An honest client" << endl;
            } 
            else {
                cout << "Imposter!" << endl;
            }

            // cleanup and pointer dedanglification
            close(clientSd);
            clientEnt = NULL;
            clientIp = NULL;
            currentIp = NULL;
            exit(EXIT_SUCCESS);
        } 

        close(clientSd);
    } 

    close(serverSd);
    return 0;
} // end main(int, char**)