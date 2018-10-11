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
#include <map>
#include "toydes.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 8
#define DEFAULT_PORT "2017"

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
std::map<int, int> keychain;

std::ifstream keyfile;

/*
 * Modulous power a^x mod q
 * per Fermats little theorem
 */
int powmod(int a, int x, int q)
{
    unsigned int r = 1;
    x = x%q;
    while(x > 0)
    {
        if (x&0x1)
        {
            r = (r*a)%q;
        }
        x = x>>1;
        a = (a*a)%q;
    }
    return r;
}

int distributeSessionKey()
{
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
        idA = (recvbuf[0] << 8) + (recvbuf[1]&0xff);
        idB = (recvbuf[2] << 8) + (recvbuf[3]&0xff);
        //tmp = t.decryptByte(recvbuf[0], key);

    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (keychain.find(idA) == keychain.end())
    {
        std::cerr << "No records of ID: " << idA << std::endl;
        return 1;
    }
    if (keychain.find(idB) == keychain.end())
    {
        std::cerr << "No records of ID: " << idB << std::endl;
        return 1;
    }
    keyA = keychain[idA];
    keyB = keychain[idB];

    long n1 = ((t.decryptByte(recvbuf[4], keyA)&0xff) << 24) + ((t.decryptByte(recvbuf[5], keyA)&0xff) << 16) + ((t.decryptByte(recvbuf[6], keyA)&0xff) << 8) + (t.decryptByte(recvbuf[7], keyA)&0xff);

    std::cout << "User with id " << idA << " requests key for communication with user with id " << idB << std::endl;

    if ((std::time(0) - n1) > 300){
        std::cout << "Error invalid timestamp " << std::time(0) << " " << n1 << std::endl;
        return 1;
    }

    // At this point we know that there are records of both A and B, so it is time to generate the session key, and prepare the package to send back to A
    srand(std::time(NULL));
    int ks = rand()&0x3ff;

    std::cout << "Generated session key " << idA << " -> " << idB << ": " << ks << " " << t.cts(ks, 10) << std::endl;

    // The returned package is E_Ka[ Ks || IDB || N1 || E_Kb[ Ks || IDA || N1 ]]
    // --------------> 6b padding + 10b  || 2B || 4B || ( 6b padding + 10b || 2B || 4B ) = 16B
    sendbuf[0] = t.encryptByte((ks >> 8)&0xff, keyA);
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
    //std::cout << t.cts(sendbuf[0], 8) << " " << t.cts(sendbuf[1], 8) << std::endl;

    iSendResult = send(clientSocket, sendbuf, 16, 0);
    if (iSendResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
}

int newUser()
{
    int p;
    int q;
    int ya;
    int yb;
    int idA;
    Toydes t = Toydes();

    iResult = recv(clientSocket, recvbuf, 6, 0);
    if (iResult > 0)
    {
        p = ((recvbuf[0]&0xff) << 8) + (recvbuf[1]&0xff);
        q = ((recvbuf[2]&0xff) << 8) + (recvbuf[3]&0xff);
        ya = ((recvbuf[4]&0xff) << 8) + (recvbuf[5]&0xff);
    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    //std::cout << p << " " << q << " ";
    srand(std::time(NULL));
    int x = rand()%q;

    //std::cout << ya << std::endl;
    yb = powmod(p, x, q);

    sendbuf[0] = (yb >> 8)&0xff;
    sendbuf[1] = yb&0xff;
    idA = keychain.size() + 1;
    sendbuf[2] = (idA >> 8)&0xff;
    sendbuf[3] = idA&0xff;

    iSendResult = send(clientSocket, sendbuf, 4, 0);
    if (iSendResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    keychain[idA] = powmod(ya, x, q)&0x3ff;
    std::cout << "Assigned id " << idA << " key: " << keychain[idA] << " " << t.cts(keychain[idA], 10) << std::endl;

    return 0;
}

int __cdecl main(int argc, char* argv[])
{
	if (argc < 2)
    {
		std::cerr << "usage: " << argv[0] << " port" << std::endl;
		exit(1);
	}

    port = argv[1];

    while(1)
    {
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

        iResult = recv(clientSocket, recvbuf, 1, 0);
        if (iResult > 0)
        {
            if (recvbuf[0] == 'd') // User requests session key
            {
                distributeSessionKey();
            }
            if (recvbuf[0] == 'e') // User requests to establish initial key
            {
                newUser();
            }
        }
        else if (iResult < 0)
        {
            std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
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
    }
    return 0;
}