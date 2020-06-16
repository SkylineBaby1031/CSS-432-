#include "Socket.h"
#include <stdlib.h>

using namespace std;

void server();
void usage(char progName[]);

Socket* sock;
//Error arg checking
int main(int argc, char* argv[]) {
    if (argc > 1) {
        sock = new Socket(atoi(argv[1]));
        if (argc == 2)
            server();
    }
    else {
        usage(argv[0]);
        return -1;
    }
    return 0;
}
//Running the server
void server() {

    // Get a server sd
    int serverSd = sock->getServerSocket();

    // Exchange data
    char message[1500];
    //First send a 10B data segment
    read(serverSd, message, 10);
    //Read 10B data segment from client
    write(serverSd, message, 10);
    //Send a FIN to client and shut off transmissions
    shutdown(serverSd, SHUT_WR);
    //Read the 1450 byte message
    read(serverSd, message, 1450);
    // Close socket but not send FIN.
    close(serverSd);
}


void usage(char progName[]) {
    cerr << "usage:" << endl;
    cerr << "server invocation: " << progName << " ipPort" << endl;
}
