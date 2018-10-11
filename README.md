# Cryptography I Key Distribution Homework 2

This assignment consists of two programs, a server to act as a trusted Key Distribution Center, and a client that can request keys from the server, as well as communicate with other clients.

## Project Details
The program is written in C++, and makes use of the WinSock2 Library, as well as the [Toydes class](https://github.com/doskoj/crypto-hw1) from the previous assignment, as well as a modified random prime generator found [here](http://www.cplusplus.com/forum/beginner/21769/).
This project acts on a number of very important assumptions.

### Key Distribution Center Implementation
The server is implemented in kdc.cpp which imports the Toydes Class mentioned above. Using the Winsock2 Library, the server listens on a port for incomming connections. The server has two main functions that it carries out, key generation for a specific user using the Diffie-Hellman Key Exchange Protocol, and acting as a trusted authority to distribute a session key for two clients, using the Needham-Schroeder Symmetric Key Protocol.

#### Diffie-Hellman Key Exchange Implementation
If the server receives a character indicating that the client connecting will be requesting to generate a key, the server will move into a function to add create the new key, and add the key to its key chain. The server then listens on the connected port and receives three values from the client. The first two are numbers ```p``` and ```q```, where ```q``` is a "large" prime and ```p``` is a primitive root of ```q```. The third value is ```ya```, the clients public key for this exchange. The server then chooses a random value for its private key ```x``` making sure to pick a value less than ```q```. Then it performs two calculations, first its public key ```yb = p^x mod q```. Then it calulates the final key ```ks = ya^x mod q```, and stores it its keychain, assigning a new ID for this transaction. The server responds to the client providing it with the values of ```yb``` and the ID that it has been assigned.

#### Needham-Schroeder Implementation
If the server receives a character indicating tha thte client connecting will be requesting a shared session key, the server then moves to the appropriate function. The server receives three values fromt the client, the clients ID, the ID that the client wishes to communicate with, and a nonce that consists of a timestamp encrypted with the requesting clients key. First the server confirms that it does in fact have valid keys for the two IDs. THen it validates the recieved nonce, making sure that it originated in the last 5 minutes. Then the server generates a random integer, and takes the last 10 bits as the session key for the two clients. It then makes two packets each containing the session key, ID of the other client, and the timestamp. Each of these packets is encrypted with the repective key for each client. Both packets are then sent to the requesting client.

### Client Implementation
The client is implemented in client.cpp, which works very much the same way as the server, using the Toydes Class and WinSock2 Library. The client opperates on a command line system that allows the user to make different requests. The two most important requests for this program are to generate a key for communication with the Key Distribution Center using the Diffie-Hellman Key Exchange Protocol, and requesting a session key from the Key Distribution Center for communication with another client, using the Needham-Schroeder Symmetric Key Protocol.

#### Diffie-Hellman Key Exchange Implementation
The client connects to the server and indicates that it is would like to generate a new key by sending the appropriate character. Then the client generates several numbers. The first number is ```q``` which has go be a "large" prime, and is generated using the RandomPrimeGenerator class. The next value to be generated is ```p``` which must be a primitive root of ```q```. This calulation is done by generating random values, less than ```q``` until it finds a value that shows that ```p^((q-1)/2) = -1 mod q```. Due to the fact that ```q``` is prime, we know that no matter what value we choose for ```p```, ```gcd(p, q) = ```. Then the program picks its private key ```x``` and calculates the public key ```ya = p^x mod q```. It then sends the three values, ```p```, ```q```, and ```ya``` to the server and awaits a response. The response it receives contains the servers public key ```yb``` and an ID for use in future trasactions. Finally the program calculates the final key ```ks = yb^x mod q```.

#### Needham-Schroeder Implementation

##### Sending
The client connects to the server and indicates that it would like a key for use with another client. Then the client sends the ID of itself, the ID of the client it wishes to communicate with, and an encrypted timestamp, using the previously established key with the Key Distribution Center. It waits for a response from the server that contains two encrypted packets. The first one contains the information for this program. It validates the received ID and timestamp from the server, and then adds the decrypted key to its keychain, associated with the ID it wants to communicate with. Then it connects to the other client, and sends other encrypted packet to them. The program then waits to receive a timestamp from the other client, which it validates, runs through a hashing algorithm ```f()``` and sends the response. All the validation communication is encrypted using the new session key.

##### Receiving
If the client receives a request from another client, it will decrypt it using the key for the Key Distribution Center. Then the timestamp is checked to ensure it is recent, and if it passes, the key is added to the keychain with the received ID. Then the client encrypts a timestamp using the new session key and sends it as a reply. It receives a response which is checked to ensure that it matches the response from the function ```f()```. If this is accepted, the validation succeeds and the clients can now communicate. If it fails the key is deleted and the program alerts the user.

## Usage

### Building the Project
The project is compiled into two separate programs, the client and the server, using g++. The program uses Windows Sockets, so it is important to be compiled on a computer that supports this. The client is compiled with
```
g++ client.cpp toydes.cpp randomPrimeGenerator.cpp -o client.exe -lws2_32
```
The server is compiled the same way
```
g++ kdc.cpp toydes.cpp -o kdc.exe -lws2_32
```

### Running the Program
Each program is run from commandline.

#### Key Distribution Center Server
The server is run using
```
$ ./kdc <port>
```
This designates the port for the server to listen on. All communication will be done through this port.

#### Client
The client is run using
```
$ ./client <server-port> <client-port>
```
Thes designates the ports used for communicating with the Key Distribution Center, and for communicating with other clients. From here there are a number of commands that can be used.

##### Generating a New Key
```
>> generatekey <server-address>
```
This tells the program to attempt to generate a new key for communication with the Key Distribution Center that has the indicated IP address.

##### Requesting a Session Key
```
>> getsessionkey <receiver-id> <server-address> <receiver-address>
```
This tells the program to ask the Key Distribution Center for a session key to be used with the client with the appropriate ID, and the indicated IP address.

##### Accepting Session Keys
```
>> accept
```
This tells the program to listen for communications with other clients, that will contain a new session key for that client. It then verifies that it was made through the Key Distribution Center.

##### Exiting the Program
```
>> exit
```

## Authors

* **Jacob Doskocil** 