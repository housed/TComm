#include "Server.h"

Server::Server() 
{
	result_ = NULL;
	ZeroMemory(&hints_, sizeof(hints_));
	hints_.ai_family = AF_INET;
	hints_.ai_socktype = SOCK_STREAM;
	hints_.ai_protocol = IPPROTO_TCP;
	hints_.ai_flags = AI_PASSIVE;

	listen_socket_ = INVALID_SOCKET;

	ZeroMemory(&client_sockets_, sizeof(client_sockets_));
	for ( int i = 0; i < MAXCONNECTIONS; i++ ) 
	{
		client_sockets_[i] = INVALID_SOCKET; 
	}
}

Server::~Server() 
{

}

int Server::Execute() 
{
	Initialize();
	Run();
	Shutdown();	
	return 0;
}

int main(int argc, char* argv[]) 
{
	Server server;
	server.Execute();
	return 0;
}

int Server::Initialize()
{
	SetConsoleTitle("Server");
	InitializeWinsock();
	ResolveServer();
	CreateListenSocket();
	BindListenSocket();
	return 0;
}

int Server::Run() 
{	
	HandleConnections();
	return 0;
}

void Server::Shutdown() 
{
	closesocket(listen_socket_);
	WSACleanup();
}

int Server::InitializeWinsock() 
{
	WSADATA wsaData;
	int wsaStatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if ( wsaStatus != 0 ) 
	{
		printf("WSAStartup failed with error: %d\n", wsaStatus);
		return 1;
	}

	return 0;
}

int Server::ResolveServer() 
{
	int result = getaddrinfo(NULL, DEFAULT_PORT, &hints_, &result_);
	if ( result != 0 ) 
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}

	return 0;
}

int Server::CreateListenSocket() 
{
	listen_socket_ = socket(result_->ai_family, result_->ai_socktype, result_->ai_protocol);
	if ( listen_socket_ == INVALID_SOCKET ) 
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result_);
		WSACleanup();
		return 1;
	}

	return 0;
}

int Server::BindListenSocket() 
{
	int result = bind(listen_socket_, result_->ai_addr, (int)result_->ai_addrlen);
	if ( result == SOCKET_ERROR ) 
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result_);
		closesocket(listen_socket_);
		WSACleanup();
		return result;
	}

	freeaddrinfo(result_);

	return 0;
}

int Server::HandleConnections() 
{
	std::cout << "Start: Server::HandleIncomingConnection()\n";

	_beginthread(SendData, 0, &client_sockets_);	

	while ( 1 ) 
	{
		/*
		for ( int i = 0; i < g_num_clients_connected; i++ ) 
		{
			if ( client_sockets_[i] == INVALID_SOCKET ) 
			{
				std::cout << "A client has disconnected.\n";
				std::cout << "Swapping " << client_sockets_[i] << " with " << client_sockets_[g_num_clients_connected - 1] << "\n";
				std::swap(client_sockets_[i], client_sockets_[g_num_clients_connected - 1]);
				g_num_clients_connected--;
			}
		}
		

		if ( g_num_clients_connected >= MAXCONNECTIONS ) 
		{
			continue;
		}
		*/
		
		ListenForConnection();
		AcceptConnection();

		std::cout << ++g_num_clients_connected << " successful connections.\n" << std::endl;

		Connection client;
		client.socket = client_sockets_;
		client.id = g_num_clients_connected - 1;

		_beginthread(ReceiveData, 0, &client);
	}

	std::cout << "End: Server::HandleIncomingConnection()\n";

	return 0;
}

int Server::ListenForConnection() 
{
	int result = listen(listen_socket_, SOMAXCONN);
	if ( result == SOCKET_ERROR ) 
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listen_socket_);
		WSACleanup();
		return result;
	}

	return 0;
}

int Server::AcceptConnection() 
{
	client_sockets_[g_num_clients_connected] = accept(listen_socket_, NULL, NULL);
	if ( client_sockets_[g_num_clients_connected] == INVALID_SOCKET )
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(listen_socket_);
		WSACleanup();
		return 1;
	}

	return 0;
}

void Server::SendData(void *info) 
{
	std::cout << "Start: Server::SendData()\n";

	SOCKET *client_socket = (SOCKET *)info;
	SOCKET *start = client_socket;
	SOCKET *temp = NULL;
	int result = 0;	

	while ( 1 ) 
	{		
		for ( int i = 0; i < g_num_clients_connected; i++ ) 
		{
			temp = client_socket + (g_num_clients_connected - 1);
			
			//std::cout << "Address of client_socket[" << i << "]: " << client_socket << "\n"; 
			//std::cout << "Value of client_socket[" << i << "]: " << *client_socket << "\n";		

			if ( *client_socket != INVALID_SOCKET ) 
			{
				result = send(*client_socket, NULL, 0, 0);
				//result = send(*client_socket, "This string was sent from Server::SendData().\n", DEFAULT_BUFLEN, 0);
				if ( result == SOCKET_ERROR ) 
				{
					printf("send() failed with error: %d\n", WSAGetLastError());
				}
			}
			else if ( *client_socket == INVALID_SOCKET )
			{
				std::cout << "A client has disconnected.\n";
				std::cout << "Swapping " << client_socket << " with " << temp << "\n";
				std::swap(*client_socket, *temp);
				g_num_clients_connected--;
				break;
			}	
		}

		client_socket, temp = start;
	} 
	
	std::cout << "End: Server::SendData()\n";
}

void Server::ReceiveData(void *info) 
{
	Connection *client = (Connection *)info;
	int result = 0;

	std::cout << "A Server::ReceiveData() thread has started for client_id " << client->id << "\n";

	do {
		result = recv(client->socket[client->id], NULL, 0, 0);
		//result = recv(client->socket[client->id], NULL, DEFAULT_BUFLEN, 0);		
		if ( result > 0 ) 
		{ 
			std::cout << "Bytes received: " << result << "\n";
		}
		else if (result == 0)
		{
			std::cout << "Connection closing...\n";
		}
		else 
		{
		    printf("recv() failed with error: %d\n", WSAGetLastError());
		}
	} while ( result > 0 );

	result = shutdown(client->socket[client->id], SD_SEND);
	if ( result == SOCKET_ERROR ) 
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}

	closesocket(client->socket[client->id]);
	client->socket[client->id] = INVALID_SOCKET;

	std::cout << "client_id " << client->id << " was shutdown and closed.\n";
}