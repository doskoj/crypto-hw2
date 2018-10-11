# Cryptography I Key Distribution Homework 2

This assignment consists of two programs, a server to act as a trusted Key Distribution Center, and a client that can request keys from the server, as well as communicate with other clients.

## Project Details
The program is written in C++, and makes use of the WinSock2 Library, as well as the [Toydes class](https://github.com/doskoj/crypto-hw1) from the previous assignment, as well as a modified random prime generator found [here](http://www.cplusplus.com/forum/beginner/21769/).

### Key Distribution Center Implementation
The server is implemented in kdc.cpp which imports the Toydes Class mentioned above. Using the Winsock2 Library, the server listens on a port for incomming connections. The server has two main functions that it carries out, key generation for a specific user using the Diffie-Hellman Key Exchange Protocol, and acting as a trusted authority to distribute a session key for two clients, using the Needham-Schroeder Symmetric Key Protocol.

#### Diffie-Hellman Key Exchange Implementation
If the server receives a character indicating that the client connecting will be requesting to generate a key, the server will move into a function to add create the new key, and add the key to its key chain. The server then listens on the connected port and receives three values from the client. The first two are numbers ```p``` and ```q```, where ```q``` is a "large" prime and ```p``` is a primitive root of ```q```. The third value is ```ya```, the clients public key for this exchange. The server then chooses a random value for its private key ```x``` making sure to pick a value less than ```q```. Then it performs two calculations, first its public key ```yb```=```p```^```x``` mod ```q```

### Client Implementation
The client is implemented in client.cpp, which works very much the same way as the server, using the Toydes Class and WinSock2 Library. The client creates a socket and attempts to connect to the port and server provided by the user, and with a successful connection, sends the file specified by the user that is encrypted byte by byte using the key provided. Once it is done sending the file, the connection is closed.

## Usage

### Building the Project
The project is compiled into two separate programs, the client and the server, using g++. The program uses Windows Sockets, so it is important to be compiled on a computer that supports this. The client is compiled with
```
g++ client.cpp toydes.cpp -o client.exe -lws2_32
```
The server is compiled the same way
```
g++ server.cpp toydes.cpp -o server.exe -lws2_32
```

### Running the Program
Each program is run from commandline, specifying the arguments to use. The server must be started first.
```
$ ./server <port> <file-name> <key>
```
This designates the port for the server to listen on, the file name under which to save the received file, as well as the key that it will use to decrypt any incomming messages. Next the client is run.
```
$ ./client <server-address> <port> <file-name> <key>
```



## Authors

* **Jacob Doskocil** 