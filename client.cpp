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
#include "randomPrimeGenerator.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 16
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
int iSendResult;
int recvbuflen = DEFAULT_BUFLEN;
unsigned int key = 917;
char packet[8];
const char* server;
unsigned int idS;
unsigned int idR;
unsigned int id;
std::map<int, int> keychain; // Stores all session keys by id

SOCKET listenSocket = INVALID_SOCKET;
SOCKET peerSocket = INVALID_SOCKET;

void testInit() // Initial parameters for testing distribution
{
    if (id == 1)
    {
        key = 917;
    }
    if (id == 2)
    {
        key = 172;
    }
}

long f(long in)
{
    return in;
}

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

int genPrimitiveRoot(int q)
{
    int x = rand()%q;
    int y = (q-1)/2;
    srand(std::time(NULL));
    while(1)
    {
        //std::cout << "Trying " << x << std::endl;
        x = (x+1)%q; // Random number less than q
        if (powmod(x, y, q) == (q - 1)) // Test if primitive root
        {
            return x;
        }
    }
}

int generateKey(const char* port)
{
    int p;
    int q;

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

    sendbuf[0] = 'e';

    // Tell the kdc you are generating a key
    iResult = send(connectSocket, sendbuf, 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    RandomPrimeGenerator rpg = RandomPrimeGenerator();
    rpg.init_fast(32768);
    p = rpg.get();
    q = rpg.get();
    //std::cout << p << " " << q << " ";
    int x = genPrimitiveRoot(q);

    int ya = powmod(p, x, q);
    //std::cout << ya << std::endl;
    int yb;

    sendbuf[0] = (p >> 8)&0xff;
    sendbuf[1] = p&0xff;
    sendbuf[2] = (q >> 8)&0xff;
    sendbuf[3] = q&0xff;
    sendbuf[4] = (ya >> 8)&0xff;
    sendbuf[5] = ya&0xff;

    // Send P and Q
    iResult = send(connectSocket, sendbuf, 6, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    iResult = recv(connectSocket, recvbuf, 4, 0);
    if (iResult > 0)
    {
        yb = ((recvbuf[0]&0xff)<< 8) + (recvbuf[1]&0xff);
        id = ((recvbuf[2]&0xff) << 8) + (recvbuf[3]&0xff);
    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    key = powmod(yb, x, q)&0x3ff;

    Toydes t = Toydes();
    std::cout << "Key generated: " << key << " " << t.cts(key, 10) << std::endl;
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

    sendbuf[0] = 'd';
    iResult = send(connectSocket, sendbuf, 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    Toydes t = Toydes();
    char tmp;
    long time = std::time(0);
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

    keychain[idR] = ((t.decryptByte(recvbuf[0], key) << 8)&0x3) + (t.decryptByte(recvbuf[1], key)&0xff);
    //std::cout << "Key: " << t.cts(keychain[idR], 10) << std::endl;

    for (int i = 0; i < 8; i++)
    {
        packet[i] = recvbuf[i+8];
    }

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

int sendSessionKey(const char* port, const char* peer)
{
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
    iResult = getaddrinfo(peer, port, &hints, &result);
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
        std::cerr << "Unable to connect to peer" << std::endl;
        WSACleanup();
        return 1;
    }

    iResult = send(connectSocket, packet, 8, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    Toydes t = Toydes();
    iResult = recv(connectSocket, recvbuf, 4, 0);
    if (iResult > 0)
    {
        // Timestamp calc?
    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    long time = (t.decryptByte(recvbuf[0], keychain[idR]) << 24) + (t.decryptByte(recvbuf[1], keychain[idR]) << 16) + (t.decryptByte(recvbuf[2], keychain[idR]) << 8) + t.decryptByte(recvbuf[3], keychain[idR]);

    if ((std::time(0) - time) > 300)
    {
        std::cerr << "Error: invalid time stamp" << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    long response = f(time);

    sendbuf[0] = t.encryptByte((response >> 24)&0xff, keychain[idR]);
    sendbuf[1] = t.encryptByte((response >> 16)&0xff, keychain[idR]);
    sendbuf[2] = t.encryptByte((response >> 8)&0xff, keychain[idR]);
    sendbuf[3] = t.encryptByte((response)&0xff, keychain[idR]);

    iResult = send(connectSocket, sendbuf, 4, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

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

int accept(const char* port)
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

    peerSocket = accept(listenSocket, NULL, NULL);
    if (peerSocket == INVALID_SOCKET)
    {
        std::cerr << "accept failed with error " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    Toydes t = Toydes();
    iResult = recv(peerSocket, recvbuf, 8, 0);
    if (iResult > 0)
    {
        idS = ((t.decryptByte(recvbuf[2], key)&0xff)<<8) + t.decryptByte(recvbuf[3], key)&0xff;
        keychain[idS] = ((t.decryptByte(recvbuf[0], key) << 8)&0x3) + (t.decryptByte(recvbuf[1], key)&0xff);
        std::cout << "Key received from peer with id " << idS << std::endl << "Validating... ";
    }
    else if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(peerSocket);
        WSACleanup();
        return 1;
    }
    //std::cout << t.cts(recvbuf[0], 8) << " " << t.cts(recvbuf[1], 8) << std::endl;
    //std::cout << "Key: " << t.cts(keychain[idS], 10) << std::endl;
    //TEST TIMESTAMP
    long response = std::time(0);
    sendbuf[0] = t.encryptByte((response >> 24)&0xff, keychain[idS]);
    sendbuf[1] = t.encryptByte((response >> 16)&0xff, keychain[idS]);
    sendbuf[2] = t.encryptByte((response >> 8)&0xff, keychain[idS]);
    sendbuf[3] = t.encryptByte((response)&0xff, keychain[idS]);

    iResult = send(peerSocket, sendbuf, 4, 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
        closesocket(peerSocket);
        WSACleanup();
        return 1;
    }

    iResult = recv(peerSocket, recvbuf, 4, 0);
    if (iResult < 0)
    {
        std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
        closesocket(peerSocket);
        WSACleanup();
        return 1;
    }

    long received = (t.decryptByte(recvbuf[0], keychain[idS]) << 24) + (t.decryptByte(recvbuf[1], keychain[idS]) << 16) + (t.decryptByte(recvbuf[2], keychain[idS]) << 8) + t.decryptByte(recvbuf[3], keychain[idS]);
    if (received != f(response))
    {
        std::cerr << "Validation failed" << std::endl;
        keychain.erase(idS);
        closesocket(peerSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Validation succeeded" << std::endl;

    iResult = shutdown(peerSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "shutdown failed with error " << WSAGetLastError() << std::endl;
        closesocket(peerSocket);
        WSACleanup();
        return 1;
    }

    closesocket(peerSocket);
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

    if (argc > 1)
    {
        id = atoi(argv[1]);
        testInit();
    }

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
        if (command == "getsessionkey")  // usage: getsessionkey sender-id receiver-id server-address peer-address
        {
            std::cin >> in1 >> in2 >> in3 >> in4;
            idS = atoi(in1.c_str());
            idR = atoi(in2.c_str());
            server = in3.c_str();
            if (!getSessionKey("2001"))
            {
                std::cout << "Key received successfully" << std::endl;
                sendSessionKey(DEFAULT_PORT, in4.c_str());
            }
        }
        else if (command == "send") // usage: send receiver-id
        {

        }
        else if (command == "generatekey") // usage: generatekey server-address
        {
            std::cin >> in1;
            server = in1.c_str();
            generateKey("2001");
        }
        else if (command == "accept") // usage: accept port
        {
            std::cin >> in1;
            accept(in1.c_str());
        }
        else if (command == "powmod")
        {
            std::cin >> in1 >> in2 >> in3;
            std:: cout << in1 << "^" << in2 << " mod " << in3 << " = " << powmod(atoi(in1.c_str()), atoi(in2.c_str()), atoi(in3.c_str())) << std::endl;
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