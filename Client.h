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

    void handleRequest(string method, string fileName);

    struct in_addr getHostIP(string hostName);

    string recieveData(int size, string fileName);

    bool sendHeader(string data);

    void sendFile(string fileName);

    struct sockaddr_in server;
    int soc_desc;


};


#endif //UNTITLED_CLIENT_H
