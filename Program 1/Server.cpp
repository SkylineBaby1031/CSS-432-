//Lu Lu
//CSS 432 

#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <sys/time.h>
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <unistd.h>       // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <pthread.h>
#include <iostream>

int repetition;
int BUFSIZE = 1500;
int maxConnections = 5;


void *readDatabuf(void *fd) {

    char databuf[BUFSIZE];
    struct timeval startTime;
    struct timeval stopTime;
    gettimeofday(&startTime, NULL); // start timer
    int count = 0;

    for (int i = 0; i < repetition; i++) {
        for (int nRead = 0; (nRead += read(*(int *)fd, databuf, BUFSIZE - nRead)) < BUFSIZE; ++count);
    }

    write(*(int *) fd, &count, sizeof(count));
    gettimeofday(&stopTime, NULL); // end timer
    close(*(int *) fd);
    long dataReceivingTime = (stopTime.tv_sec - startTime.tv_sec) * 1000000 +
                             (stopTime.tv_usec - startTime.tv_usec);
    std::cout << "total read() call = " << count << " times" << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Invalid Input" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    repetition = 20000;

    sockaddr_in sendSockAddr;
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sendSockAddr.sin_port = htons(port);

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);

    const int on = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));

    bind(serverSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));

    listen(serverSd, maxConnections);

    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    while (true) {
        int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);

        pthread_t newThread;
        int threadVal;
        threadVal = pthread_create(&newThread, NULL, readDatabuf, (void*) &newSd);

        pthread_join(newThread, NULL);
    }

    close(serverSd);  // close server socket

    return 0;
}