#ifndef _CLIENT_H_
#define _CLIENT_H_

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <process.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define USERNAME_LEN	16
#define DEFAULT_PORT "27017"

char g_username[USERNAME_LEN] = "smashypoo";

struct MessagePacket 
{
	char username[USERNAME_LEN];
	char message[DEFAULT_BUFLEN - USERNAME_LEN];
};

struct Packet 
{
	char outbound_message[DEFAULT_BUFLEN];
	char inbound_message[DEFAULT_BUFLEN];
	SOCKET* ClientSocket;
};

void SendData(void* info) 
{
	Packet* packet = (Packet*)info;
	char msg[DEFAULT_BUFLEN] = { NULL };
	int status = 0;  

	//printf("SendData()\n"); // test

	do {
		std::cin.getline(msg, DEFAULT_BUFLEN);

		memcpy(packet->outbound_message, g_username, strlen(g_username) + 1);
		memcpy(packet->outbound_message + sizeof(g_username), msg, strlen(msg) + 1);

		//status = send(*packet->ClientSocket, packet->outbound_message, sizeof(packet->outbound_message), 0);
		if ( status == SOCKET_ERROR ) 
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(*packet->ClientSocket);
			WSACleanup();
			return;
		}
		printf("SendData(): data sent successfully!\n"); // test
	} while ( status > 0 );
}

void ReceiveData(void* info) 
{
	Packet* packet = (Packet*)info;
	char msg[DEFAULT_BUFLEN] = { NULL };
	int status = 0;

	//printf("ReceiveData()\n"); // test

	do {
		//printf("ReceiveData(), about to recv()!\n"); // test
		status = recv(*packet->ClientSocket, (char*)msg, DEFAULT_BUFLEN, 0);
		//printf("ReceiveData(), just called recv(). Did anything happen?\n"); // test
		if ( status > 0 ) 
		{
			//printf("ReceiveData(), status > 0\n"); // test
			printf("%s", msg);

			memcpy(packet->inbound_message, msg, strlen(msg) + 1);
		}
		else if ( status == 0 ) 
		{
			printf("ClientSocket closing...\n");
		}
		else 
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(*packet->ClientSocket);
			WSACleanup();
			return;
		}
	} while ( status > 0 );

	//printf("End of ReceiveData()\n"); // test
}



int main(int argc, char **argv) {
	SetConsoleTitle("Client");

	WSADATA wsaData;
	int wsaStatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if ( wsaStatus != 0 )
	{
		printf("WSAStartup failed with error: %d\n", wsaStatus);
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints = {};
	char recvbuf[DEFAULT_BUFLEN] = {};
	int status = 0;
	int recvbuflen = DEFAULT_BUFLEN;
	char ipaddr[16] = "127.0.0.1";

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	status = getaddrinfo(ipaddr, DEFAULT_PORT, &hints, &result);
	if ( status != 0 ) 
	{
		printf("getaddrinfo failed with error: %d\n", status);
		WSACleanup();
		return 1;
	}

	for ( ptr = result; ptr != NULL; ptr = ptr->ai_next )  // Attempt to connect to an address until one succeeds 
	{ 
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // Create a SOCKET for connecting to server
		if ( ConnectSocket == INVALID_SOCKET ) 
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		else { // Connect to server.
			status = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if ( status == SOCKET_ERROR ) 
			{
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue; // Start next iteration of this for()
			}
			else 
			{
				freeaddrinfo(result);

				if ( ConnectSocket == INVALID_SOCKET ) 
				{
					printf("Unable to connect to server! %d\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}
				else 
				{
					printf("Connected to server.\n");
					break;
				}
			}
		}
	}

	HANDLE handle[2];
	Packet packet;
	packet.ClientSocket = &ConnectSocket;

	handle[0] = (HANDLE)_beginthread(SendData, 0, &packet);
	handle[1] = (HANDLE)_beginthread(ReceiveData, 0, &packet);
	WaitForMultipleObjects(2, handle, false, INFINITE);

	status = shutdown(ConnectSocket, SD_SEND);
	if ( status == SOCKET_ERROR ) 
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

#endif