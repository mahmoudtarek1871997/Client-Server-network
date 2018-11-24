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
#include <unistd.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent
#include <queue>

using namespace std;

class Client {
public:
    Client();
    bool conToserver(string hostName, int port);
    /**
     *
     * @param req
     * @return 1 if post, -1 if not valid, 0 if fin or get
     */
    int handleRequest(string req);
    void closeSocket();
    struct in_addr getHostIP(string hostName);
    bool sendRequest(string data);
    void sendFile(string fileName, int len);
    int soc_desc;
    queue<string> fileNames;
    string postFileName;
    pthread_t recvThread;
    int fileLen;
    /*
      * send FIN signal to server
      * */
    void sendCloseSignal();

    void handleGET(string message, string fileName);

    void handlePOST(string message, string fileName, int len);

    void handleFIN(string message);

    vector<string> split(string stringToBeSplitted, string delimeter);



    /**
     * get content length in get response
     * @return the content length or -1 if content length header not found
     * */
    int getContentLen(char *buffer, int startIndex, int recvSize);
};

bool handleRemainder(char buffer[], int i, int recvSize, Client* c);
#endif //UNTITLED_CLIENT_H