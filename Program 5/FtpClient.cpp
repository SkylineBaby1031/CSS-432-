#include "FtpClient.h"
#include <iostream>
#include <sys/poll.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

FtpClient::FtpClient() {}

//Constructor
FtpClient::FtpClient(char* server)
{
    if (server == NULL)
    {
        getServer();
    }
    else
    {
        this->server = server;
    }
    this->port = 21;
    this->connected = false;
}

//Destructor
FtpClient::~FtpClient() {}


//connect to the server with user input server name
void FtpClient::getServer()
{
    cout << "open: ";
    getline(cin, command);
    server = (char*)command.c_str();
}

//Establish a connection to ip on default port 21.
void FtpClient::ftpConnect()
{
    mainSD = ftpConnect(port);
}

//Establish a connection to ip on port.
int FtpClient::ftpConnect(int thisPort)
{
    char buffer[server.size() + 1];
    std::size_t length = server.copy(buffer, server.size(), 0);
    buffer[length] = '\0';
    struct hostent* host = gethostbyname(buffer);
    if (!host) Error("host does not exist");
    sockaddr_in sendSockAddr;
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*) * host->h_addr_list));
    sendSockAddr.sin_port = htons(thisPort);

    //create socket
    const int clientSd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSd == -1)
    {
        Error("socket() call error");
    }

    //connect to server
    const int connected = connect(clientSd, (sockaddr*)&sendSockAddr, sizeof(sendSockAddr));

    if (connected == -1)
    {
        Error("connect() call error, host does not exist");
    }

    //connection
    if (mainSD == -1)
    {
        readResponse(clientSd, false);
        mainSD = clientSd;
        string userString(getlogin());
        cout << "Name (" << server << ":" << userString << "): ";
    }
    return clientSd;
}

//Read response
int FtpClient::readResponse(int sd, bool savePassivePort)
{
    pollfd ufds;
    ufds.fd = sd;
    ufds.events = POLLIN;
    ufds.revents = 0;
    char errorCode[3];
    bzero(errorCode, 3);
    int count = 0;
    string ipString;
    bool ipFound = false;
    int rc = poll(&ufds, 1, 1000);
    int bytesRead = 1;
    char buf[1];
    while (rc > 0 && bytesRead != 0)
    {
        bytesRead = read(sd, buf, 1);
        if (bytesRead > 0)
        {
            if (savePassivePort && (buf[0] == '(' || ipFound))
            {
                ipFound = true;
                ipString += buf[0];
            }
            if (count < 3)
            {
                errorCode[count] = buf[0];
            }
            count++;
            cout << buf[0];
        }
        rc = poll(&ufds, 1, 1000);
    }
    if (savePassivePort) parsePassive(ipString);
    return atoi(errorCode);
}

//login with username and password
void FtpClient::logIn()
{
    getline(cin, this->command);                    //read username and send USER command
    string username = "USER " + command + '\n';
    char* toSend = (char*)username.c_str();
    write(mainSD, toSend, username.length());
    readResponse(mainSD, false);

    bool loggedIn = false;
    while (!loggedIn)
    {
        cout << "Password: ";
        getline(cin, this->command);                //read password, send PASS command
        string username = "PASS " + command + '\n';
        char* toSend = (char*)username.c_str();
        write(mainSD, toSend, username.length());
        int rc = 0;
        while (rc == 0)rc = readResponse(mainSD, false);
        if (rc == LOG_IN_TIMEOUT)
        {
            exit(1);
        }
        loggedIn = (rc == LOGGED_IN);
    }

    string systCall = "SYST\n";                     //SYST command
    char* syst = (char*)systCall.c_str();
    write(mainSD, syst, systCall.length());
    readResponse(mainSD, false);

    connected = true;
}

//start getting program and get user command
void FtpClient::start()
{
    while (true)
    {
        getUserCommand();
    }
}


//Running the program and get user command input
//Command required for the program
void FtpClient::getUserCommand()
{
    cout << "ftp> ";
    getline(cin, this->command);
    int wordCount = count(command.begin(), command.end(), ' ') + 1;
    string parsed[wordCount];
    char* word = strtok((char*)command.c_str(), " ");

    //parse the user command
    for (int i = 0; i < wordCount; i++)
    {
        parsed[i] = word;
        word = strtok(NULL, " ");
    }

    if (parsed[0] == "open" && !connected && wordCount == 2)
    {
        server = parsed[1];
        ftpConnect();
        logIn();
    }
    if (parsed[0] == "cd" && connected && wordCount > 1)
    {
        cdSubDir(parsed[1]);
    }
    else if (parsed[0] == "ls" && connected)
    {
        ls();
    }
    else if (parsed[0] == "get" && connected && wordCount > 1)
    {
        getFile(parsed[1]);
    }
    else if (parsed[0] == "put" && connected && wordCount > 1)
    {
        putFile(parsed[1]);
    }
    else if (parsed[0] == "close")
    {
        closeFtp();
    }
    else if (parsed[0] == "quit")
    {
        quit();
    }
    else
    {
        cout << "Not recognized\n";
    }

}

//Using server IP for connect
void FtpClient::openFtp(string server)
{
    this->server = (char*)server.c_str();
    ftpConnect();                                   //connect
}


//parse the passive port
void FtpClient::parsePassive(string ipString)
{
    ipString = ipString.substr(1, ipString.length() - 2);
    int wordCount = count(ipString.begin(), ipString.end(), ',') + 1;
    string parsed[wordCount];
    char* word = strtok((char*)ipString.c_str(), ",");

    for (int i = 0; i < wordCount; i++)
    {
        parsed[i] = word;
        word = strtok(NULL, ",");
    }

    pasvPort = (256 * atoi(parsed[wordCount - 2].c_str())) + atoi(parsed[wordCount - 1].c_str());
}

//error report
void FtpClient::Error(string err)
{
    cerr << "(ERROR) ";
    cerr << err << endl;
    exit(1);
}


//change the server's current working directory to subdir
void FtpClient::cdSubDir(string dir)
{
    string dirCommand = "CWD " + dir + "\n";
    char* buf = (char*)dirCommand.c_str();
    write(mainSD, buf, dirCommand.length());
    readResponse(mainSD, false);
}

//Ask the server to send back its current directory contents through the data connection.
void FtpClient::ls()
{
    string dirCommand = "PASV\n";
    char* buf = (char*)dirCommand.c_str();
    write(mainSD, buf, dirCommand.length());
    readResponse(mainSD, true);
    pasvSD = ftpConnect(pasvPort);

    string listCommand = "LIST\n";
    char* listBuf = (char*)listCommand.c_str();
    int pid = fork();

    if (pid == 0)
    {
        readResponse(pasvSD, false);
        exit(0);
    }

    write(mainSD, listBuf, listCommand.length());
    readResponse(mainSD, false);
    wait(NULL);
    close(pasvSD);
}

//Get a file from the current remote directory.
void FtpClient::getFile(string file)
{
    string binCommand = "Type I\n";
    char* buf = (char*)binCommand.c_str();
    write(mainSD, buf, binCommand.length());
    readResponse(mainSD, false);

    string pasvCommand = "PASV\n";
    char* pasvBuf = (char*)pasvCommand.c_str();
    write(mainSD, pasvBuf, pasvCommand.length());
    readResponse(mainSD, true);
    pasvSD = ftpConnect(pasvPort);

    string listCommand = "RETR " + file + "\n";
    char* listBuf = (char*)listCommand.c_str();

    int pid = fork();

    if (pid == 0)
    {
        readFile(file);
        exit(0);
    }

    write(mainSD, listBuf, listCommand.length());
    readResponse(mainSD, false);
    wait(NULL);
    close(pasvSD);
}

//Store a file into the current remote directory.
void FtpClient::putFile(string file)
{
    string pasvCommand = "PASV\n";
    char* pasvBuf = (char*)pasvCommand.c_str();
    write(mainSD, pasvBuf, pasvCommand.length());
    readResponse(mainSD, true);
    pasvSD = ftpConnect(pasvPort);

    string binCommand = "Type I\n";
    char* buf = (char*)binCommand.c_str();
    write(mainSD, buf, binCommand.length());
    readResponse(mainSD, false);

    string listCommand = "STOR " + file + "\n";
    char* listBuf = (char*)listCommand.c_str();

    int pid = fork();

    if (pid == 0)
    {
        writeFile(file);
        exit(0);
    }

    write(mainSD, listBuf, listCommand.length());
    wait(NULL);
    close(pasvSD);
    readResponse(mainSD, false);
}


//read File
void FtpClient::readFile(string filename)
{
    std::ofstream file(filename.c_str(), ios_base::out | ios::binary);

    pollfd ufds;
    ufds.fd = pasvSD;
    ufds.events = POLLIN;
    ufds.revents = 0;

    int rc = poll(&ufds, 1, 1000);
    int bytesRead = 1;
    while (rc > 0 && bytesRead != 0)
    {
        char buf[1];
        bytesRead = read(pasvSD, buf, 1);
        if (bytesRead > 0)
        {
            file.write(buf, 1);
        }
        rc = poll(&ufds, 1, 1000);
    }
    file.close();
}

//write File
void FtpClient::writeFile(string filename)
{
    ifstream in(filename.c_str());
    string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    char* buf = (char*)contents.c_str();
    write(pasvSD, buf, contents.length());
    in.close();
}

//	close the connection but not quit ftp.
void FtpClient::closeFtp()
{
    if (!connected)
    {
        cout << "connection already closed\n";
        return;
    }

    string dirCommand = "QUIT\n";
    char* buf = (char*)dirCommand.c_str();
    write(mainSD, buf, dirCommand.length());
    int rc = readResponse(mainSD, false);
    close(mainSD);
    connected = false;
    mainSD = -1;
}

//close the connectionand quit ftp.
void FtpClient::quit()
{
    if (connected)
    {
        closeFtp();
    }
    exit(0);
}

