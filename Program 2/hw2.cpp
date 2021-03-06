#include <iostream>
#include <vector>
#include <unistd.h>		
#include "UdpSocket.h"
#include "Timer.h"
using namespace std;

const int PORT = 75118;		  // my UDP port
const int MAX = 20000;        // times of message transfer
const int MAX_WIN = 30;       // maximum window size	
const bool verbose = false;   // use verbose mode for more information during run
const int TIMEOUT = 1500;

// client packet sending functions
void ClientUnreliable(UdpSocket& sock, int max, int message[]);
int ClientStopWait(UdpSocket& sock, int max, int message[]);
int ClientSlidingWindow(UdpSocket& sock, int max, int message[], int windowSize);

// server packet receiving functions
void ServerUnreliable(UdpSocket& sock, int max, int message[]);
void ServerReliable(UdpSocket& sock, int max, int message[]);
void ServerEarlyRetrans(UdpSocket& sock, int max, int message[], int windowSize);

enum myPartType { CLIENT, SERVER } myPart;

int main(int argc, char* argv[])
{
	int message[MSGSIZE / 4]; 	  // prepare a 1460-byte message: 1460/4 = 365 ints;

	// Parse arguments
	if (argc == 1)
	{
		myPart = SERVER;
	}
	else if (argc == 2)
	{
		myPart = CLIENT;
	}
	else
	{
		cerr << "usage: " << argv[0] << " [serverIpName]" << endl;
		return -1;
	}

	// Set up communication
	// Use different initial ports for client server to allow same box testing
	UdpSocket sock(PORT + myPart);
	if (myPart == CLIENT)
	{
		if (!sock.setDestAddress(argv[1], PORT + SERVER))
		{
			cerr << "cannot find the destination IP name: " << argv[1] << endl;
			return -1;
		}
	}

	int testNumber;
	cerr << "Choose a testcase" << endl;
	cerr << "   1: unreliable test" << endl;
	cerr << "   2: stop-and-wait test" << endl;
	cerr << "   3: sliding windows" << endl;
	cerr << "--> ";
	cin >> testNumber;

	if (myPart == CLIENT)
	{
		Timer timer;
		int retransmits = 0;

		switch (testNumber)
		{
		case 1:
			timer.Start();
			ClientUnreliable(sock, MAX, message);
			cout << "Elasped time = ";
			cout << timer.End() << endl;
			break;
		case 2:
			timer.Start();
			retransmits = ClientStopWait(sock, MAX, message);
			cout << "Elasped time = ";
			cout << timer.End() << endl;
			cout << "retransmits = " << retransmits << endl;
			break;
		case 3:
			for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++)
			{
				timer.Start();
				retransmits = ClientSlidingWindow(sock, MAX, message, windowSize);
				cout << "Window size = ";
				cout << windowSize << " ";
				cout << "Elasped time = ";
				cout << timer.End() << endl;
				cout << "retransmits = " << retransmits << endl;
			}
			break;
		default:
			cerr << "no such test case" << endl;
			break;
		}
	}
	if (myPart == SERVER)
	{
		switch (testNumber)
		{
		case 1:
			ServerUnreliable(sock, MAX, message);
			break;
		case 2:
			ServerReliable(sock, MAX, message);
			break;
		case 3:
			for (int windowSize = 1; windowSize <= MAX_WIN; windowSize++)
			{
				ServerEarlyRetrans(sock, MAX, message, windowSize);
			}
			break;
		default:
			cerr << "no such test case" << endl;
			break;
		}

		// The server should make sure that the last ack has been delivered to client.

		if (testNumber != 1)
		{
			if (verbose)
			{
				cerr << "server ending..." << endl;
			}
			for (int i = 0; i < 10; i++)
			{
				sleep(1);
				int ack = MAX - 1;
				sock.ackTo((char*)&ack, sizeof(ack));
			}
		}
	}
	cout << "finished" << endl;
	return 0;
}

// Test 1 Client
void ClientUnreliable(UdpSocket& sock, int max, int message[])
{
	// transfer message[] max times; message contains sequences number i
	for (int i = 0; i < max; i++)
	{
		message[0] = i;
		sock.sendTo((char*)message, MSGSIZE);
		if (verbose)
		{
			cerr << "message = " << message[0] << endl;
		}
	}
	cout << max << " messages sent." << endl;
}

// Test1 Server
void ServerUnreliable(UdpSocket& sock, int max, int message[])
{
	// receive message[] max times and do not send ack
	for (int i = 0; i < max; i++)
	{
		sock.recvFrom((char*)message, MSGSIZE);
		if (verbose)
		{
			cerr << message[0] << endl;
		}
	}
	cout << max << " messages received" << endl;
}



int ClientStopWait(UdpSocket& sock, int max, int message[])
{
	//Implement this function
	int retransmissionCount = 0;

	Timer ackTimer;

	for (int sequenceNum = 0; sequenceNum < max; sequenceNum++)
	{
		message[0] = sequenceNum;
		sock.sendTo((char*)message, MSGSIZE);

		ackTimer.Start();

		while (sock.pollRecvFrom() < 1)
		{
			//check timeout
			if (ackTimer.End() > TIMEOUT)
			{
				sock.sendTo((char*)message, MSGSIZE);

				if (verbose)
				{
					cout << "Message #" <<
						sequenceNum <<
						" resent." <<
						endl;
				}

				ackTimer.Start();

				retransmissionCount++;
			}
		}
		sock.recvFrom((char*)message, MSGSIZE);

	}


	return retransmissionCount;

}

void ServerReliable(UdpSocket& sock, int max, int message[])
{
	//Implement this function
	for (int sequenceNum = 0; sequenceNum < max; sequenceNum++)
	{
		do
		{
			sock.recvFrom((char*)message, MSGSIZE);

			//if sequence as expected, send ack back
			if (message[0] == sequenceNum)
			{
				sock.ackTo((char*)&sequenceNum, sizeof(sequenceNum));
			}
			else if (verbose)
			{
				cout << "Got message #" <<message[0] << " from the client." <<endl;
			}		

		} while (message[0] != sequenceNum);

	}

}

int ClientSlidingWindow(UdpSocket& sock, int max, int message[], int windowSize)
{
	//Implement this function
	int retransmissionCount = 0;
	int sequenceNextFrame = 0;
	int ackSequenceLastFrame = 0;


	Timer ackTimer;
	int lastAckReceived = -1;


	while (
		(sequenceNextFrame < max) ||
		(ackSequenceLastFrame < max))
	{
		if (
			(sequenceNextFrame < max) &&
			((ackSequenceLastFrame + windowSize) > sequenceNextFrame)
			)
		{
			message[0] = sequenceNextFrame;
			sock.sendTo((char*)message, MSGSIZE);

			sequenceNextFrame++;

		}

		// read the ack from the server.
		if (sock.pollRecvFrom() > 0)
		{
			sock.recvFrom((char*)&lastAckReceived,
				sizeof(lastAckReceived));



			if (lastAckReceived == ackSequenceLastFrame)
			{
				ackSequenceLastFrame++;
			}
		}
		else
		{

			ackTimer.Start();

			while (sock.pollRecvFrom() < 1)
			{
				if (ackTimer.End() > TIMEOUT)
				{
					if (verbose)
					{
						cout << "Frame #: " <<
							ackSequenceLastFrame <<
							" timed out," <<
							" for sequence #: " <<
							sequenceNextFrame <<
							endl;
					}

					retransmissionCount +=
						(sequenceNextFrame - ackSequenceLastFrame);




					// The frame arrived late
					if (
						(lastAckReceived <=
							sequenceNextFrame) &&
						(lastAckReceived >=
							ackSequenceLastFrame)
						)
					{
						ackSequenceLastFrame =
							1 + lastAckReceived;
					}
					else
					{
						sequenceNextFrame =
							ackSequenceLastFrame;
					}

					break;


				}
			}
		}

	}


	return retransmissionCount;

}

void ServerEarlyRetrans(UdpSocket& sock, int max, int message[], int windowSize)
{
	//Implement this function
	int lastFrameReceived = 0;
	int lastAckFrame = 0;
	int lastSequence = -1;

	vector<bool> array(max, false);


	do
	{
		if (sock.pollRecvFrom() > 0)
		{
			sock.recvFrom((char*)message, MSGSIZE);
			lastFrameReceived = message[0];

			if ((lastFrameReceived - lastAckFrame) > windowSize)
			{
				continue;
			}
			else if (lastFrameReceived > lastAckFrame)
			{
				array[lastFrameReceived] = true;

				while (array[lastAckFrame])
				{
					lastSequence = lastAckFrame++;
				}

			}
			else
			{
				array[lastFrameReceived] = true;
				lastSequence = lastAckFrame;
			}



			sock.ackTo((char*)&lastSequence, sizeof(lastSequence));	

		}


	} while (lastAckFrame < max);


}