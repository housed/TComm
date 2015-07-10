#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <process.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

struct INFO
{
	char username[DEFAULT_BUFLEN];
	SOCKET ConnectSocket = INVALID_SOCKET;
};

const char USERNAME[] = "smash";

void outbound(void *info)
{
	INFO *ConnectInfo = (INFO *)info;
	SOCKET ConnectSocket = ConnectInfo->ConnectSocket;
	char username[DEFAULT_BUFLEN];
	char input[DEFAULT_BUFLEN];
	char message[DEFAULT_BUFLEN];
	char sendbuf[DEFAULT_BUFLEN];
	int iResult = 0;

	std::strcpy(sendbuf, ConnectInfo->username);
	std::strcat(sendbuf, " has connected\n");

	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return;
	}

	do
	{
		std::cin.getline(input, DEFAULT_BUFLEN);

		if (strcmp(input, "/disconnect") == 0)
		{
			// shutdown the connection since no more data will be sent
			iResult = shutdown(ConnectSocket, SD_SEND);
			if (iResult == SOCKET_ERROR)
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return;
			}
		}
		else
		{
			std::strcpy(username, ConnectInfo->username);
			std::strcat(username, ": ");
			std::strcpy(message, username);
			std::strcat(message, input);
			std::strcpy(sendbuf, message);
			// Send buffer.
			iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return;
			}

			/*printf("\nBytes sent: %d\n", iResult);

			printf("Message sent: ");
			for (int i = 0; i < iResult; i++)
			{
			printf("%c", sendbuf[i]);
			}
			printf("\n");*/
		}

	} while (iResult > 0);
}

void inbound(void *info)
{
	INFO *ConnectInfo = (INFO *)info;
	SOCKET ConnectSocket = ConnectInfo->ConnectSocket;
	char recvbuf[DEFAULT_BUFLEN];
	int iResult = 0;
	int recvbuflen = DEFAULT_BUFLEN;

	do
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			//printf("\nBytes received: %d\n", iResult);
			//printf("Message received: ");
			for (int i = 0; i < iResult; i++)
			{
				printf("%c", recvbuf[i]);
			}
			printf("\n");
		}
		else if (iResult == 0)
		{
			printf("\nConnection closed\n");
		}
		else
		{
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
		}

	} while (iResult > 0);
}

int main(int argc, char **argv)
{
	SetConsoleTitle(L"Client");

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;

	INFO ConnectInfo;
	char host[24];

	// Validate the parameters
	//if (argc != 2)
	//{
	//	printf("usage: %s server-name\n", argv[0]);
	//	return 1;
	//}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	printf("Enter a username: ");
	std::cin.getline(ConnectInfo.username, DEFAULT_BUFLEN);
	printf("Connect to: ");
	std::cin.getline(host, 24);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(host, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL;ptr = ptr->ai_next)
	{

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	ConnectInfo.ConnectSocket = ConnectSocket;
	printf("\nWelcome, %s!\n\n", ConnectInfo.username);

	HANDLE handle[2];
	handle[0] = (HANDLE)_beginthread(outbound, 0, &ConnectInfo);
	handle[1] = (HANDLE)_beginthread(inbound, 0, &ConnectInfo);
	WaitForMultipleObjects(2, handle, TRUE, INFINITE);

	// cleanup
	printf("Cleanup\n");
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}