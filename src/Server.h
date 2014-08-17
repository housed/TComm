#ifndef _SERVER_H_
#define _SERVER_H_

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

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN	512
#define USERNAME_LEN	
#define MAXCONNECTIONS  3
#define DEFAULT_PORT	"27017"

int g_num_clients_connected = 0;

struct Connection 
{
	int id;
	SOCKET *socket;
};

class Server 
{
private:	
	struct addrinfo *result_;
	struct addrinfo hints_;
	SOCKET listen_socket_;
	SOCKET client_sockets_[MAXCONNECTIONS];

public:
	Server();
	~Server();

	int Execute();
	int Initialize();
	int Run();
	void Shutdown();

	int InitializeWinsock();               // The WSAStartup function initiates use of the Winsock DLL by a process. http://msdn.microsoft.com/en-us/library/windows/desktop/ms742213(v=vs.85).aspx
	int ResolveServer();                   // Resolve the server address and port http://msdn.microsoft.com/en-us/library/windows/desktop/ms738520(v=vs.85).aspx
	int CreateListenSocket();              // The socket function creates a socket that is bound to a specific transport service provider. http://msdn.microsoft.com/en-us/library/windows/desktop/ms740506(v=vs.85).aspx
	int BindListenSocket();                // The bind function associates a local address with a socket. http://msdn.microsoft.com/en-us/library/windows/desktop/ms737550(v=vs.85).aspx
	
	int HandleConnections();               // Listen for and permit a valid incoming connection.
	int ListenForConnection();             // The listen function places a socket in a state in which it is listening for an incoming connection. http://msdn.microsoft.com/en-us/library/windows/desktop/ms739168(v=vs.85).aspx
	int AcceptConnection(); 	           // The accept function permits an incoming connection attempt on a socket. http://msdn.microsoft.com/en-us/library/windows/desktop/ms737526(v=vs.85).aspx
	
	static void ReceiveData(void *info);   // Receive data from connected client.
	static void SendData(void *info);      // Send data to connected client.
};

#endif
