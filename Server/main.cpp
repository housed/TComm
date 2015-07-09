#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <process.h>
#include <vector>
#include <iostream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

struct SHAREDDATA
{
	char recvbuf[DEFAULT_BUFLEN] = { NULL };
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult = 0;
	int iSendResult = 0; 
	std::vector<SOCKET> ClientSocket;
};

void outbound(void *info)
{
	SHAREDDATA *SharedData = (SHAREDDATA *)info;
	int iSendResult = 0;

	while (1)
	{
		if (SharedData->iResult > 0)
		{
			for (int i = 0; i < SharedData->ClientSocket.size(); i++)
			{
				iSendResult = send(SharedData->ClientSocket[i], SharedData->recvbuf, SharedData->iResult, 0);
				if (iSendResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(SharedData->ClientSocket[i]);
					SharedData->ClientSocket[i] = INVALID_SOCKET;
					SharedData->ClientSocket.erase(SharedData->ClientSocket.begin() + i);
				}
				else
				{
					printf("Bytes sent: %d\n", iSendResult);
					printf("Message sent: ");
					for (int i = 0; i < iSendResult; i++)
					{
						printf("%c", SharedData->recvbuf[i]);
					}
					printf("\n\n");
				}
			}

			SharedData->iResult = 0;
		}
	}
}

void inbound(void *info)
{
	SHAREDDATA *SharedData = (SHAREDDATA *)info;
	SOCKET ClientSocket = SharedData->ClientSocket.back();

	do {

		SharedData->iResult = recv(ClientSocket, SharedData->recvbuf, SharedData->recvbuflen, 0);

		if (SharedData->iResult > 0)
		{
			printf("Bytes received: %d\n", SharedData->iResult);

			printf("Message received: ");
			for (int i = 0; i < SharedData->iResult; i++)
			{
				printf("%c", SharedData->recvbuf[i]);
			}
			printf("\n\n");
		}
		else if (SharedData->iResult == 0)
		{
			printf("Connection closing...\n");
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			return;
		}

	} while (SharedData->iResult > 0);

	std::cout << "Looking for ClientSocket " << ClientSocket << "\n";
	for (int i = 0; i < SharedData->ClientSocket.size(); i++)
	{
		std::cout << i << ": " << SharedData->ClientSocket[i] << "\n";
	}

	for (int i = 0; i < SharedData->ClientSocket.size(); i++)
	{
		if (ClientSocket == SharedData->ClientSocket[i])
		{
			printf("Found it!\n");
			SharedData->ClientSocket.erase(SharedData->ClientSocket.begin() + i);
		}
	}

	// shutdown the connection since we're done
	SharedData->iResult = shutdown(ClientSocket, SD_SEND);
	if (SharedData->iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		ClientSocket = INVALID_SOCKET;
		return;
	}

	closesocket(ClientSocket);
	ClientSocket = INVALID_SOCKET;

	printf("Connection closed\n\n");
}

int main(void)
{
	SetConsoleTitle("Server");

	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	SHAREDDATA SharedData;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	_beginthread(outbound, 0, &SharedData);

	while (1)
	{
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		SharedData.ClientSocket.push_back(ClientSocket);

		_beginthread(inbound, 0, &SharedData);
	}

	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}