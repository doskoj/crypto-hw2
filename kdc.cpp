#define _WIN32_WINNT 0x501

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <ctime>
#include <random>
#include "toydes.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 8
#define DEFAULT_PORT "2017"

int disctributeSessionKey()
{

}

int newUser()
{

}

int __cdecl main(int argc, char* argv[])
{
	WSADATA wsaData;
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;	

	unsigned int key;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
    char sendbuf[16];
	char* port;
	char* fname;

    std::ifstream keyfile;

    // ID file, contains 2 byte ID (stored in hexadecimal) and 10 bit key (stored in binary)
    keyfile.open("kdckeys.txt");

    //std::cout << itoa(std::time(0), tmp, 16 );

	if (argc < 2)
    {
		std::cerr << "usage: " << argv[0] << " port" << std::endl;
		exit(1);
	}

    port = argv[1];

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        std::cerr << "WSAStartup failed with error: " <<  iResult << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0)
    {
        std::cerr << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
    	std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
    	freeaddrinfo(result);
    	closesocket(listenSocket);
    	WSACleanup();
    	return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
    	std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
    	closesocket(listenSocket);
    	WSACleanup();
    	return 1;
    }

    clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
    	std::cerr << "accept failed with error " << WSAGetLastError() << std::endl;
    	closesocket(listenSocket);
    	WSACleanup();
    	return 1;
    }

    // First receive is IDA || IDB || E_Ka[ N1 ] -> 2B || 2B || 4B -> 8 Bytes
    Toydes t = Toydes();
    unsigned char tmp[8];
    unsigned int idA;
    unsigned int idB;
    unsigned int keyA;
    unsigned int keyB;

	iResult = recv(clientSocket, recvbuf, 8, 0);
	if (iResult > 0)
	{
        idA = (recvbuf[0] << 8) + recvbuf[1];
        idB = (recvbuf[2] << 8) + recvbuf[3];
        //n1 = (recvbuf[4] << 24) + (recvbuf[5] << 16) + (recvbuf[6] << 8) + recvbuf[7];
		//tmp = t.decryptByte(recvbuf[0], key);

	}
	else if (iResult < 0)
	{
		std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

//    std::cout << t.cts(recvbuf[0], 8) << " " << t.cts(recvbuf[1], 8) << " " << t.cts(recvbuf[2], 8) << " " << t.cts(recvbuf[3], 8) << std::endl;
    char tmp2[5];
    char tmp3[11];
    bool found[] = {false, false};
//  std::cout << idA << " " << idB << " " << n1 << std::endl;
    while ((keyfile >> tmp2 >> tmp3) && !(found[0] && found[1]))
    {
//        std::cout << tmp2 << " " << tmp3 << std::endl; 
        if (strtol(tmp2, NULL, 16) == idA){
            keyA = strtol(tmp3, NULL, 2);
            found[0] = true;
        }
        else if (strtol(tmp2, NULL, 16) == idB){
            keyB = strtol(tmp3, NULL, 2);
            found[1] = true;
        }
    }

    if (!found[0])
    {
        std::cerr << "No records of ID: " << idA << std::endl;
        return 1;
    }
    if (!found[1])
    {
        std::cerr << "No records of ID: " << idB << std::endl;
        return 1;
    }

    // At this point we know that there are records of both A and B, so it is time to generate the session key, and prepare the package to send back to A
    int ks = 0;
    for (int i = 0; i < 10; i++)
    {
        //Choose each of the 10 bits as a random number
        ks += (rand()%2 << i);
    }

    // The returned package is E_Ka[ Ks || IDB || N1 || E_Kb[ Ks || IDA || N1 ]]
    // --------------> 6b padding + 10b  || 2B || 4B || ( 6b padding + 10b || 2B || 4B ) = 16B
    sendbuf[0] = t.encryptByte((ks >> 8)&0x3, keyA);
    sendbuf[1] = t.encryptByte((ks&0xff), keyA);
    sendbuf[2] = t.encryptByte((idB >> 8)&0xff, keyA);
    sendbuf[3] = t.encryptByte((idB)&0xff, keyA);
    sendbuf[4] = recvbuf[4];
    sendbuf[5] = recvbuf[5];
    sendbuf[6] = recvbuf[6];
    sendbuf[7] = recvbuf[7];
    sendbuf[8] = t.encryptByte((ks >> 8)&0xff, keyB);
    sendbuf[9] = t.encryptByte((ks&0xff), keyB);
    sendbuf[10] = t.encryptByte((idA >> 8)&0xff, keyB);
    sendbuf[11] = t.encryptByte((idA)&0xff, keyB);
    sendbuf[12] = t.encryptByte(t.decryptByte(recvbuf[4], keyA), keyB);
    sendbuf[13] = t.encryptByte(t.decryptByte(recvbuf[5], keyA), keyB);
    sendbuf[14] = t.encryptByte(t.decryptByte(recvbuf[6], keyA), keyB);
    sendbuf[15] = t.encryptByte(t.decryptByte(recvbuf[7], keyA), keyB);

    iSendResult = send(clientSocket, sendbuf, 16, 0);
    if (iSendResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
    	std::cerr << "shutdown failed with error " << WSAGetLastError() << std::endl;
    	closesocket(clientSocket);
    	WSACleanup();
    	return 1;
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}