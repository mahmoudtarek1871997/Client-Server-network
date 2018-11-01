//
// Created by zook on 01/11/18.
//

//#ifndef CLIENT_SERVER_HTTP_SERVER_H
//#define CLIENT_SERVER_HTTP_SERVER_H
//
//#endif //CLIENT_SERVER_HTTP_SERVER_H

#include<iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>


using namespace std;

class Server {
public:
    Server();

    int sock_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1500];
    struct sockaddr_in serv_addr, cli_addr;

    /**
     * create socket file descriptor, return true if the file descriptor successfully created, -1 otherwise
     */
    bool createSocketFD();

    /**
     * bind the socket to the current IP address on port "portno"
     * @param portno
     * @return true if successful , false otherwise
     */
    bool bindServer(int portno);

    /**
     * places all incoming connection into a backlog queue until accept() call accepts the connection.
     * @param queue_size, the maximum size for the backlog queue
     */
    void listenToCon(int queue_size);

    /**
     * function will write the connecting client's address info
     * into the the address structure and the size of that structure is clilen.
     * @param cli_addr
     * @param cli_len
     * @return a new socket file descriptor for the accepted connection, if failed return -1
     */
    int acceptCon(sockaddr_in cli_addr, socklen_t cli_len);

    /**
     * send data
     * @param data
     * @param socket
     * @return
     */
    bool sendHeader(int socket, string data);

    /**
     *
     * @param size
     * @param fileName
     * @return the data or empty string if error occured
     */
    string recieveData(int socket, int size, string fileName);

    /**
     *
     * @param socket
     */
    void closeCon(int socket);

};