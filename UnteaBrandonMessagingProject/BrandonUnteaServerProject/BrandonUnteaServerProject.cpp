// defining and include all the libraries being used
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <iostream>

// prototypes
int ReData(SOCKET socket, char* data, int length);
int SendData(SOCKET socket, char* buffer, int size);

// global socket and port
SOCKET serSocket;
uint16_t port = 0;

// chat capacity and user list
std::unordered_map<SOCKET, std::string> users;
int chatCapacity;
int userCount = 0;

// global file
std::ofstream logFile;

// UDP SOCKETS Work
SOCKET UDPSocket;
void UDPHelper(int port);

int main()
{
    // UDP Message
    std::string UDPMessage = "127.0.0.1\n31337";

    // declare necessary variables for the server
    fd_set master;
    fd_set backupSet;
    char buffer[256];
    std::string name;
    int clients;
    WSADATA wsaData;

    // opening up a file to log
    logFile.open("log.txt", std::ios::out | std::ios::app);

    // initializing winsock
    WSAStartup(WINSOCK_VERSION, &wsaData);

    //UDP Set up
    UDPHelper(UDPSocket);
    int port = 31337;
    sockaddr_in UDPAddr;
    UDPAddr.sin_family = AF_INET;
    UDPAddr.sin_addr.S_un.S_addr = inet_addr("255.255.255.255");
    UDPAddr.sin_port = htons(port);

    // adding title to the server
    printf("----------Server----------\n");

    // prompting to enter the port and saving the port
    printf("Please enter the port you would like to use. \n");
    printf("Port: ");
    std::cin >> port;

    printf("Enter the chat capacity for this server. \n");
    printf("Capacity: ");
    std::cin >> chatCapacity;

    // creating a new socket
    serSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // setting up the address strut for the server socket
    sockaddr_in serAddr;
    serAddr.sin_port = htons(port);
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    // bind the server socket to the address
    bind(serSocket, (SOCKADDR*)&serAddr, sizeof(serAddr));

    // start the listening to incoming connections
    listen(serSocket, 1);

    // initialize the file descriptor sets
    FD_ZERO(&master);
    FD_ZERO(&backupSet);
    FD_SET(serSocket, &master);

    // printing the waiting message
    printf("Waiting....\n");

    // Showing the user what the UDP Message is
    std::cout << "\nUDP Message: \n";
    std::cout << UDPMessage;

    //UDP Timer for one second
    timeval UDPTimer;
    UDPTimer.tv_sec = 1;
    UDPTimer.tv_usec = 0;
    
    // oop to handle all the connections and messages
    while (true) {
        // make a backup copy of the master set
        backupSet = master;

        // wait for activity on the sockdets in the backup set
        clients = select(0, &backupSet, NULL, NULL, &UDPTimer);

        // Before checking the clients send the UDP Message
        int UDPErr = sendto(UDPSocket, UDPMessage.c_str(), sizeof(UDPMessage), 0, (sockaddr*)&UDPAddr, sizeof(sockaddr));
        UDPErr = WSAGetLastError();

        // if there is an error close the server socket ending program
        if (clients == -1) {
            closesocket(serSocket);
            return 0;
        }
        // if no activity continue to wait for activity
        else if (clients == 0) {
            continue;
        }

        // if the server socket has activity, accept connection
        if (FD_ISSET(serSocket, &backupSet)) {
            SOCKET tempSocket = accept(serSocket, NULL, NULL);
            if (tempSocket != INVALID_SOCKET) {
                FD_SET(tempSocket, &master);
                printf("User has Connected to the chat...........\n");
            }
        }

        // check for activity on other sockets in the set
        for (uint8_t i = 0; i < backupSet.fd_count; i++) {
            // if the socket is a server socket continue to
            if (backupSet.fd_array[i] == serSocket) {
                continue;
            }

            // getting the message sizee
            uint8_t size = 0;
            int total = recv(backupSet.fd_array[i], (char*)&size, 1, 0);
            // check for client disconnected
            if ((total == SOCKET_ERROR) || (total == 0)) {
                // removes the socket from the master set
                FD_CLR(backupSet.fd_array[i], &master);
                // seeing if the user was in the map
                auto it = users.find(backupSet.fd_array[i]);
                if (it != users.end()) {
                    // taking the user out of the list and map
                    std::cout << users[backupSet.fd_array[i]] << " exited the server\n";
                    users.erase(backupSet.fd_array[i]);
                    // updating the user count
                    userCount--;
                }
                //closes that socket
                closesocket(backupSet.fd_array[i]);
            }
            else {
                // message has gone through
                // buffer for the message
                char buffer[256];
                ZeroMemory(buffer, 256);
                total = ReData(backupSet.fd_array[i], buffer, size);
                if ((total == SOCKET_ERROR) || (total == 0)) {
                    int err = WSAGetLastError();
                    printf("Error: ");
                    printf("%i\n", err);
                    return 0;
                }
                
                // turns the buffer into a string to check for commands
                std::string string = buffer;
                // checks if the message is possible of being the register command
                if (string.size() >= 9) {
                    if (string.substr(0,9) == "$register") {
                        if (userCount < chatCapacity) {
                            // loggin the command
                            auto now = std::chrono::system_clock::now();
                            auto now_c = std::chrono::system_clock::to_time_t(now);
                            logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;

                            // geting the name of user
                            name = string.substr(9, string.size());
                            std::cout << name << " " << "has been registered!\n";
                            logFile << std::ctime(&now_c) << name << " " << "has been registered!\n";
                            // putting it on list
                            users.insert(std::pair<SOCKET, std::string>(backupSet.fd_array[i], name));
                            // add to the user count
                            userCount++;

                            // sends the client buffer for done
                            std::string doneMessage = "Done";
                            SendData(backupSet.fd_array[i], &doneMessage[0], doneMessage.size());

                        }
                        // if the capacity is full
                        else {
                            // little error message
                            // logs the messsage and error
                            auto now = std::chrono::system_clock::now();
                            auto now_c = std::chrono::system_clock::to_time_t(now);
                            logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;

                            std::string fullMessage = "Sorry but the capacity to register is currently full try again when someone leaves!!!!\n";
                            std::cout << fullMessage;
                            logFile << std::ctime(&now_c) << "Sorry but the capacity to register is currently full try again when someone leaves!!!!\n";
                            
                            // sends the message to the user
                            SendData(backupSet.fd_array[i], &fullMessage[0], fullMessage.size());
                        }
                    }
                    else {
                        // loggin the message to the file
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);
                        logFile << std::ctime(&now_c) << "Message: " << buffer << std::endl;

                        // printing the message
                        printf("%s\n", buffer);
                    }
                }
                // checks if its possible to be a $getList
                else if (string.size() >= 8) {
                    if (string.substr(0,8) == "$getlist") {
                        if (users.size() != 0) {
                            // loggin the command
                            auto now = std::chrono::system_clock::now();
                            auto now_c = std::chrono::system_clock::to_time_t(now);
                            logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;

                            // makes a message of a list of users
                            std::string userList = "List of Users: \n";

                            // loops through the list and sees who to add to the message
                            for (const auto& temp : users) {
                                const std::string& listName = temp.second;
                                userList += listName;
                                userList += '\n';
                            }

                            // sends the list of users to the client
                            SendData(backupSet.fd_array[i], &userList[0], userList.size());

                            // messages the server that it sent the message
                            std::cout << "List of users sent\n";

                            // logs the message
                            logFile << std::ctime(&now_c) << "List of users sent\n";
                        }
                        else {
                            // loggin the command
                            auto now = std::chrono::system_clock::now();
                            auto now_c = std::chrono::system_clock::to_time_t(now);
                            logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;

                            // Sending empty list to user
                            std::string userList = "List of Users: \n";
                            userList += "THERE ARE NO USERS REGISTERED";
                            SendData(backupSet.fd_array[i], &userList[0], userList.size());

                            // messages the server that it sent the message
                            std::cout << "List is empty warned the client! ERROR ERROR \n";

                            // logs the messsage
                            logFile << std::ctime(&now_c) << "List is empty warned the client!!!\n";
                        }
                    }
                    else {
                        // little error message
                        // logs the messsage and error
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);
                        logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;

                        std::cout << "Sorry but the capacity to register is currently full try again when someone leaves!!!!\n";
                        logFile << std::ctime(&now_c) << "Sorry but the capacity to register is currently full try again when someone leaves!!!!\n";
                    }
                }
                // checks if its possible to be a $getlog
                else if (string.size() >= 7) {
                    if (string.substr(0,7) == "$getlog") {

                        std::ifstream logSend("logfile.txt");
                        if (logSend) {
                            // gets the length of the file
                            logSend.seekg(0, std::ios::end);
                            int logLength = logSend.tellg();
                            logSend.seekg(0, std::ios::beg);

                            // send the length o fthe log file
                            std::string lengthString = std::to_string(logLength);
                            send(backupSet.fd_array[i], lengthString.c_str(), lengthString.size() + 1, 0);

                            // send each line of the log file
                            std::string line;
                            while (std::getline(logSend, line)) {
                                send(backupSet.fd_array[i], lengthString.c_str(), line.size() + 1, 0);
                            }

                            // close the file
                            logSend.close();
                        }
                        // error check
                        else {
                            std::string err = "Error opening log file try again later.\n";
                            send(backupSet.fd_array[i], err.c_str(), err.size() + 1, 0);
                        }

                    }
                    else {
                        // loggin the message to the file
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);
                        logFile << std::ctime(&now_c) << "Message: " << buffer << std::endl;

                        // printing the message
                        printf("%s\n", buffer);
                    }
                }
                // checks if its possible to be a $exit
                else if (string.size() >= 5) {
                    if (string.substr(0,5) == "$exit") {

                        // loggin the command
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);
                        logFile << std::ctime(&now_c) << "Comand: " << buffer << std::endl;
                    }
                    else {
                        // loggin the message to the file
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);
                        logFile << std::ctime(&now_c) << "Message: " << buffer << std::endl;

                        // printing the message
                        printf("%s\n", buffer);
                    }
                }
                else {
                    // loggin the message to the file
                    auto now = std::chrono::system_clock::now();
                    auto now_c = std::chrono::system_clock::to_time_t(now);
                    logFile << std::ctime(&now_c) << "Message: " << buffer << std::endl;

                    // printing the message
                    printf("%s\n", buffer);
                }
            }
        }
    }
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

// Helper Function for UDP
void UDPHelper(int port) {
    // vars 
    bool UDPBool = true;

    // create a udp socket
    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(UDPSocket, SOL_SOCKET, SO_BROADCAST, (char*)&UDPBool, sizeof(UDPBool));

    // set up socket address
    sockaddr_in helperAddr;
    helperAddr.sin_port = htons(port);
    helperAddr.sin_family = AF_INET;
    helperAddr.sin_addr.S_un.S_addr = INADDR_ANY;

    // bind the udp socket to the address and port
    bind(UDPSocket, (SOCKADDR*)&helperAddr, sizeof(helperAddr));
}
