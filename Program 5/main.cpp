#include "FtpClient.h"

using namespace std;

int main(int argc, char** argv)
{
    FtpClient client(argv[1]);
    client.ftpConnect();
    client.logIn();
    client.start();

    return 0;
}