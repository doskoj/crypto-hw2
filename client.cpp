#define _WIN32_WINNT 0x501

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <string>
#include <ctime>
#include <map>
#include "toydes.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 12
#define DEFAULT_PORT "2700"
#define DEFAULT_HOST "129.161.250.221"

WSADATA wsaData;
SOCKET connectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;
char sendbuf[32];
char recvbuf[DEFAULT_BUFLEN];
int iResult;
int recvbuflen = DEFAULT_BUFLEN;
unsigned int key = 917;
char* fname;
const char* server;
unsigned int idS;
unsigned int idR;
std::map<int, int> keychain; // Stores all session keys by id

int generateKey()
{

}

int getSessionKey(const char* port)
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(server, port, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    ptr = result;
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connectSocket == INVALID_SOCKET)
    {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
    }

    if (connectSocket == INVALID_SOCKET)
    {
        std::cerr << "Unable to connect to server" << std::endl;
        WSACleanup();
        return 1;
    }

    Toydes t = Toydes();
    char tmp;
    int time = std::time(0);
//      sendbuf[0] = t.encryptByte(tmp, key);

    sendbuf[0] = ((idS >> 8) & 0xff);
    sendbuf[1] = (idS & 0xff);
    sendbuf[2] = ((idR >> 8) & 0xff);
    sendbuf[3] = (idR & 0xff);
    sendbuf[4] = t.encryptByte((time<<24)&0xff, key);
    sendbuf[5] = t.encryptByte((time<<16)&0xff, key);
    sendbuf[6] = t.encryptByte((time<<8)&0xff, key);
    sendbuf[7] = t.encryptByte((time)&0xff, key);

    iResult = send(connectSocket, sendbuf, 8, 0);
    //std::cout << t.cts(sendbuf[0], 8) << " " << t.cts(sendbuf[1], 8) << " " << t.cts(sendbuf[2], 8) << " " << t.cts(sendbuf[3], 8) << std::endl;
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        if (t.decryptByte(recvbuf[2], key) != sendbuf[2] || t.decryptByte(recvbuf[3], key) != sendbuf[3])
        {
            std::cerr << "Error: invalid response from Key Distribution Center" << std::endl;
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    keychain[idR] = ((recvbuf[0] << 8) + (recvbuf[1]))&0x3ff;

    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "shutdown failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    closesocket(connectSocket);
    WSACleanup();
    return 0;
}

int sendMessage()
{

}

int main (int argc, char* argv[])
{
    /*
    if (argc < 6)
    {
    	std::cerr << "usage: " << argv[0] << " server-address port sender-id receiver-id key" << std::endl;
    	exit(1);
    }


    char* port = argv[2];
    if (atoi(port) < 0 || atoi(port) > 65535)
    {
    	std::cerr << "ERROR: Invalid port number" << std::endl;
    	exit(1);
    }


    server = argv[1];
    key = atoi(argv[5])&0x3ff;

    idS = atoi(argv[3]);
    idR = atoi(argv[4]);
    */
    std::string command;
    std::string in1;
    std::string in2;
    std::string in3;
    std::string in4;
    std::cout << "Program Initiated" << std::endl;
    while(1)
    {
        std::cout << ">> ";
        std::cin >> command;
        if (command == "getsessionkey")  // usage: getsessionkey sender-id receiver-id port server-address
        {
            std::cin >> in1 >> in2 >> in3 >> in4;
            idS = atoi(in1.c_str());
            idR = atoi(in2.c_str());
            server = in4.c_str();
            if (!getSessionKey(in3.c_str()))
            {
                std::cout << "Key received successfully" << std::endl;
            }
        }
        else if (command == "send") // usage: send receiver-id
        {

        }
        else if (command == "generatekey") // usage: generatekey server-address
        {

        }
        else if (command == "accept") // usage: accept port
        {

        }
        else if (command == "exit")
        {
            exit(1);
        }
        else
        {
            std::cout << "Invalid command" << std::endl;
        }
    }
}