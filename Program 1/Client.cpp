//Lulu
//CSS 432 

#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <unistd.h>       // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <string>
#include <iostream>
#include <sys/time.h>

int main(int argc, char *argv[])
{
    int port = 0, repetition = 0, nbufs = 0, bufsize = 0, type = 0;
    const char* serverIp;

    if (argc == 7) {
        serverIp = argv[1] + '\0';
        port = atoi(argv[2]);
        repetition = atoi(argv[3]);
        nbufs = atoi(argv[4]);
        bufsize = atoi(argv[5]);
        type = atoi(argv[6]);

    }
    else{
        std::cerr << "Invalid Input" << std::endl;
        return 1;
    }

    struct hostent* host = gethostbyname(serverIp);


    sockaddr_in sendSockAddr;
    bzero((char *)&sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    connect(clientSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));

    struct timeval startTime;
    struct timeval lapTime;
    struct timeval endTime;

    gettimeofday(&startTime, NULL); // starting time

    char databuf[nbufs][bufsize];


    for (int i = 0; i < repetition; i++) {
        // multiple writes
        if (type == 1) {
            for (int j = 0; j < nbufs; j++) {
                write(clientSd, databuf[j], bufsize);
            }
        }
        // writev
        else if (type == 2) {
            struct iovec vector[nbufs];

            for (int j = 0; j < nbufs; j++) {
                vector[j].iov_base = databuf[j];
                vector[j].iov_len = bufsize;
            }
            writev(clientSd, vector, nbufs);
        }
        // single write
        else if (type == 3) {
            write(clientSd, databuf, nbufs * bufsize);
        }
    }
    gettimeofday(&lapTime, NULL);

    long lapTimeResult = (lapTime.tv_sec - startTime.tv_sec) * 1000000 +
                                           (lapTime.tv_usec - startTime.tv_usec);
    int buffer = 0;
    read(clientSd, &buffer, sizeof(buffer));

   
    std::cout << "Test " << type << ": time = " <<
    lapTimeResult << "usec, #reads = " << buffer << std::endl;
    close(clientSd);

    return 0;
}