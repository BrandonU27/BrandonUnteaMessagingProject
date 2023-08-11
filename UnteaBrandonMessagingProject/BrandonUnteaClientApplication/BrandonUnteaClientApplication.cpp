// Define preprocessor directives and include necessary libraries
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <string>
#include <fstream>
#include <iostream>

// Declare a global socket variable
SOCKET cliSocket;
SOCKET UDPSocket;

// register var
bool registered = false;

// Function prototypes
void SetUp(char* add, int port);
void CinClears();
int SendData(SOCKET socket, char* buffer, int size);
int ReData(SOCKET socket, char* data, int length);

int main()
{
    // Declare and initialize variables
    std::string addr = "";
    std::string name = "";
    std::string message;
    uint16_t port = 0;

    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(WINSOCK_VERSION, &wsaData);

    // Set the SO_REUSEADDR option to allow binding to the same port after a previous connection has closed
    bool helper = true;
    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int UDPResults = setsockopt(UDPSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&helper, sizeof(helper));
    
    // Set up the socket address structure
    sockaddr_in UDPAddr;
    UDPAddr.sin_family = AF_INET;
    UDPAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    UDPAddr.sin_port = htons(31337);

    // Bind the socket to the address and port
    // mutable buffer to store received data
    char buffer[100] = { 0 };  
    int UDPAddrSize = sizeof(UDPAddr);
    int UDPBind = bind(UDPSocket, (sockaddr*)&UDPAddr, sizeof(UDPAddr));

    // Wait for a UDP datagram to be received
    int UDPRecv = recvfrom(UDPSocket, buffer, 100, 0, (sockaddr*)&UDPAddr, &UDPAddrSize);
    int UDPErr = WSAGetLastError();

    // If no datagram is received, prompt the user for the IP address and port to connect to
    if (UDPRecv <= 0) {
        // Prompt the user for the IP address and port to connect to
        printf("----------Client----------\n");
        printf("Enter the IP Address to connect to: ");
        std::cin >> addr;
        CinClears();
        printf("----------Client----------\n");
        printf("Enter the Port to connect to: ");
        std::cin >> port;
        CinClears();
    }
    else {
        // Parse the received data to get the IP address and port
        buffer[UDPRecv] = '\0';
        std::string UDPMessage(buffer);
        addr = UDPMessage.substr(0, UDPMessage.find('\n'));
        UDPMessage.erase(0, (UDPMessage.find('\n') + 1));
        try {
            port = std::stoi(UDPMessage);
        }
        catch (const std::invalid_argument& e) {
            // Handle invalid input (e.g. non-numeric input)
            std::cerr << "Invalid port number: " << UDPMessage << std::endl;
            // Prompt the user for a valid port number
        }
    }

    // Connect to the server
    SetUp(&addr[0], port);

    // checks the users register
    while (!registered) {

        CinClears();
        //printf("Commands:\n$register (for this one type username after (Ex: $register Brandon))\n$getlist\n$getlog\n$exit\nEnter message:\n");
        printf("Commands:\n$register (for this one type username after (Ex: $register Brandon))\n$getlist\n$exit\nEnter message:\n");
        fflush(stdin);
        std::getline(std::cin, message);

        // tells the user that they need to register
        if (message.substr(0, 9) != "$register") {
            std::cout << "You cannot do anything until you are registered!\n Use the $register command\n";
            system("pause");
        }
        // if the register comand is played then lets the command go through
        else {
            SendData(cliSocket, &message[0], message.size());

            uint8_t size = 0;
            int total = recv(cliSocket, (char*)&size, 1, 0);
            char buffer[256];
            ZeroMemory(buffer, 256);
            total = ReData(cliSocket, buffer, size);

            std::string string = buffer;
            // checks if full
            if (string != "Sorry but the capacity to register is currently full try again when someone leaves!!!!\n") {
                registered = true;   
            }
            else {
                std::cout << string;
                shutdown(cliSocket, SD_BOTH);
                closesocket(cliSocket);
                system("pause");
                return 0;
            }
        }
    }

    // Continue prompting the user for messages until they enter the "$exit" command
    while (message != "$exit") {
        CinClears();
        //printf("Commands:\n$register (for this one type username after (Ex: $register Brandon))\n$getlist\n$getlog\n$exit\nEnter message:\n");
        printf("Commands:\n$register (for this one type username after (Ex: $register Brandon))\n$getlist\n$exit\nEnter message:\n");
        fflush(stdin);
        std::getline(std::cin, message);

            // If the user entered the "$getlist" command, send it to the server and receive a list of connected users
            if (message == "$getlist") {
                SendData(cliSocket, &message[0], message.size());

                uint8_t size = 0;
                int total = recv(cliSocket, (char*)&size, 1, 0);
                char buffer[256];
                ZeroMemory(buffer, 256);
                total = ReData(cliSocket, buffer, size);

                std::string string = buffer;
                printf("%s\n", buffer);

                // Prompt the user to continue after viewing the list of users
                std::cout << "When you are done looking at the users please press any button to continue.\n";
                system("pause");
            }
            // If the user entered the "$getlog" command, TODO: implement this command
            else if (message == "$getlog") {

                std::cout << "I am sorry I havn't figued out how to make this work for the client\n The server side I got it to work\n but sending it to the client I couldn't figure it out\n";
                std::cout << "Press anything to continue";
                system("pause");

            }
            // Otherwise, send the user's message to the server
            else {
                SendData(cliSocket, &message[0], message.size());
            }
    }

    // If the user entered the "$exit" command, clean up and exit the program
    if (message == "$exit") {
        printf("Exit.........");
        printf("Thank you for signing in have a nice day.");
        shutdown(cliSocket, SD_BOTH);
        closesocket(cliSocket);
    }
}

// function to set up the socket and connect to the server
void SetUp(char* add, int port) {
    // Create a TCP socket
    cliSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Set up the client address with the given IP address and port number
    sockaddr_in cliAddr;
    cliAddr.sin_port = htons(port);
    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = inet_addr(add);

    // Connect the socket to the server using the client address
    int cliConnect = connect(cliSocket, (SOCKADDR*)&cliAddr, sizeof(cliAddr));

    // If connection fails, print error message
    if (cliConnect != 0) {
        printf("NO SERVER FOUND");
    }
}

// Clear input buffer and clear console screen
void CinClears() {
    std::cin.clear();
    system("cls");
}

// Sends data over a socket
int SendData(SOCKET socket, char* buffer, int size) {
    int message; // variable to store the return value of send()
    int bytes = 0; // variable to keep track of how many bytes have been sent

    // Send the size of the data first
    message = send(socket, (char*)&size, 1, 0);
    if ((message == SOCKET_ERROR) || (message == 0)) {
        // If there was an error sending the size, print an error message and return
        int errorCode = WSAGetLastError();
        printf("Message has error please try again\n");
        return 1;
    }

    // Send the data in chunks until all of it has been sent
    while (bytes < size) {
        // Send a chunk of data
        message = send(socket, (const char*)buffer + bytes, size - bytes, 0);
        if ((message == SOCKET_ERROR) || (message == 0)) {
            // If there was an error sending the data, print an error message and return
            int errorCode = WSAGetLastError();
            printf("Message has an error in it\n");
            return 1;
        }
        if (message <= 0) {
            // If send() returns 0 or a negative number, the connection has been closed, so return that value
            return message;
        }
        // Update the number of bytes sent so far
        bytes += message;
    }
    // Return the total number of bytes sent
    return bytes;
}


// receiving data function
int ReData(SOCKET socket, char* data, int length) {
    int get = 0;

    // looping till all the data is received
    while (get < length) {
        int rec = recv(socket, data + get, length - get, 0);
        // error occur
        if (rec < 1) {
            return rec;
        }
        else {
            // update the number of bytes
            get += rec;
        }
    }
    // return the message
    return get;
}
