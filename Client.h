//
// Created by yassmin on 31/10/18.
//

#ifndef UNTITLED_CLIENT_H
#define UNTITLED_CLIENT_H

#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<string>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent

using namespace std;

class Client {
public:
    Client();
    bool conToserver(string hostName, int port);
    void handleRequest(string req);
    void closeSocket();
    struct in_addr getHostIP(string hostName);
    string receiveResponse(int size, string fileName);
    bool sendRequest(string data);
    void sendFile(string fileName);
    int soc_desc;

    /*
     * send FIN signal to server
     * */
    void sendCloseSignal();

    void handleGET(string message, string fileName);

    void handlePOST(string message);

    void handleFIN();

    vector<string> split(string stringToBeSplitted, string delimeter);

    /**
     * get content length in get response
     * @return the content length or -1 if content length header not found
     * */
    int getContentLen(char *buffer, int startIndex, int recvSize);
};


#endif //UNTITLED_CLIENT_H
