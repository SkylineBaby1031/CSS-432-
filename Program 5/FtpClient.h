#pragma once
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string>

using namespace std;

class FtpClient
{
public:
    FtpClient();
    FtpClient(char*);
    void ftpConnect();
    void logIn();
    void start();
    virtual ~FtpClient();
private:
    int ftpConnect(int);
    void getUserCommand();
    int readResponse(int, bool);
    void readFile(string);          //read File
    void writeFile(string);         //write to File
    void Error(string);             //report error message
    void openFtp(string);           //Establish a TCP connection to ip on port.
    void cdSubDir(string);          //change the server's current working directory to subdir
    void ls();                      //Ask the server to send back its current directory contents through the data connection.
    void getFile(string);           //Get a file from the current remote directory.
    void putFile(string);           //Store a file into the current remote directory.
    void parsePassive(string);      //PASV
    void closeFtp();                //close the connection but not quit ftp.
    void quit();                    //close the connection and quit ftp.
    void getServer();               //connect to the server with user input server name
    string server;
    int port;                       //default port 21
    int pasvSD;
    int mainSD = -1;
    int pasvPort;
    bool connected;
    string command;

    const int LOGGED_IN = 230;
    const int LOG_IN_TIMEOUT = 421;
};
