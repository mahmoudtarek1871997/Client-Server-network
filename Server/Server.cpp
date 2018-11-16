//
// Created by zook on 01/11/18.
//

#include <arpa/inet.h>
#include<pthread.h>
#include <vector>
#include <sstream>
#include <fstream>
#include "Server.h"

Server::Server() {}

vector<string> Server::split(string stringToBeSplitted, string delimeter) {
    vector<string> splittedString;
    int startIndex = 0;
    int endIndex = 0;
    while ((endIndex = stringToBeSplitted.find(delimeter, startIndex)) < stringToBeSplitted.size()) {
        string val = stringToBeSplitted.substr(startIndex, endIndex - startIndex);
        if (!val.empty())
            splittedString.push_back(val);
        startIndex = endIndex + delimeter.size();
    }
    if (startIndex < stringToBeSplitted.size()) {
        string val = stringToBeSplitted.substr(startIndex);
        if (!val.empty())
            splittedString.push_back(val);
    }
    return splittedString;
}

bool Server::createSocketFD() {
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cout << "Creating socket failed" << endl;
        return false;
    }
    return true;
}


bool Server::bindServer(int portno) {

    // server byte order
    serv_addr.sin_family = AF_INET;
    // automatically be filled with current host's IP address
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // convert short integer value for port must be converted into network byte order
    serv_addr.sin_port = htons(portno);
    if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Binding failed";
        return false;
    }
    return true;
}

void Server::listenToCon(int queue_size) {
    listen(sock_fd, queue_size);
}

int Server::acceptCon() {
    int len = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr;
    // will write the client addr and len into the sent parameters
    new_socket = accept(sock_fd, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
    if (new_socket < 0) {
        cout << "Accepting connection failed" << endl;
        return -1;
    }
    cout << "server: got connection from " << inet_ntoa(cli_addr.sin_addr) << " port "
         << ntohs(cli_addr.sin_port) << endl;
    return new_socket;
}

bool Server::sendHeader(int socket, string data) {
    return send(socket, data.c_str(), strlen(data.c_str()), 0) > 0;
}

void Server::handleGET(int soc, string fileName) {
    ifstream f(fileName.c_str());
    if(f.good()) // if file exists
        sendFile(fileName, soc);
    else{
        string message = "HTTP/1.0 404 Not Found\r\n\r\n";
        sendHeader(soc, message);
    }
}

void Server::handlePOST(int soc, string fileName) {
    string response = "HTTP/1.0 200 OK\r\n\r\n";
    if (sendHeader(soc, response)) {
        cout << "POST OK sent\n";
        cout << "recieve data....." << endl;
        recieveData(soc, 1024, fileName);
        cout << "recieving data finished!" << endl;
    } else {
        cout << "Sending POST OK ack failed!" << endl;
    }
}

void Server::handleFIN(int soc) {
    if (sendHeader(soc, "FINACK\r\n\r\n")) {
        cout << "closing requst is acknoweldged \n";
        closeCon(soc);
    } else {
        cout << "Sending FINACK failed!" << endl;
    }
}

void Server::parseRequest(int soc, string req) {
    cout<<"request received: \n"<< req <<endl;

    vector<string> lines = split(req, "\r\n");
    stringstream ss(lines[0]);
    vector<string> result;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ' ');
        result.push_back(substr);
    }
    string method = result[0];

    string fileName = "";
    if(method != "FIN")
        fileName = result[1];
    if (method == "GET") {
        handleGET(soc, fileName);
    } else if (method == "POST") {
        handlePOST(soc, fileName);
    } else if (method == "FIN") {
       handleFIN(soc);
    } else {
        cout << "Invalid request!" << endl;
    }
}

string Server::recieveData(int socket, int size, string fileName) {
    FILE *fp = fopen(fileName.c_str(), "w");
    char buffer[size];
    string data = "";
    ssize_t recvSize;
    do {
        memset(buffer, 0, sizeof(buffer));
        recvSize = recv(socket, buffer, sizeof(buffer), 0);
        cout<<recvSize<<endl;
        if (recvSize > 0) {
            data += buffer;
            fwrite(buffer, sizeof(char), recvSize, fp);
            fflush(fp);
        }
    } while (recvSize > 0);
    fclose(fp);
    return data;
}

void Server::closeCon(int socket) {

    close(socket);
    cout<<"Connection closed!"<<endl;

}

void Server::startServer(int queueSize) {
    pthread_t threads[queueSize];
    int i = 0;
    while (1) { // run forever
        struct sockaddr_in cli_add;
        socklen_t cli_len;
        serverArgs server_args;

        server_args.socket = acceptCon();
        server_args.server = this;

        int creation_result = pthread_create(&threads[i++], NULL, socketThread, &server_args);
        if (creation_result != 0)
            printf("Failed to create thread with error number : %d\n", creation_result);

        // if maximum queue size is reached, wait for connections to finish
        if (i >= queueSize) {
            for (i = 0; i < queueSize; i++)
                pthread_join(threads[i++], NULL);
            i = 0;
        }

    }
}

void *socketThread(void *arg) {

    serverArgs *args = ((serverArgs *) arg);
    int socket = args->socket;
    Server *server = args->server;
//    for(int i = 0 ; i < 5 ; i++) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    string req = "";
    recv(socket, buffer, sizeof(buffer), 0);
    req = buffer;

    server->parseRequest(socket, req);
//    }

    memset(buffer, 0, sizeof(buffer));
    req = "";
    recv(socket, buffer, sizeof(buffer), 0);
    req = buffer;
    server->parseRequest(socket, req);
//
    pthread_exit(NULL);

}

void Server::sendFile(string fileName, int soc) {
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    FILE *fp = fopen(fileName.c_str(), "r");
    int read = 0;
    read = fread(buff, 1, sizeof(buff), fp);
    string message = "HTTP/1.1 200 OK\r\nContent-Length: 1080\r\n\r\n";
    message += buff;
    message = message.substr(0,message.size()-1); // remove the last end char in buff
    memset(buff, 0, sizeof(buff));
    while ((read = fread(buff, 1, sizeof(buff), fp)) > 0) {
        for(int i = 0; i < read ; i++){
            message += buff[i];
        }
        memset(buff, 0, sizeof(buff));
    }
    sendHeader(soc, message);
    fclose(fp);
}




#define port 8080

int main() {
    cout << "Hello, Servser! \n";
    Server *server = new Server();
    server->createSocketFD();
    cout << "socket created \n";
    bool res = server->bindServer(port);
    cout << "binding finished " << res << endl;
    server->listenToCon(50);
    cout << "Listening .." << endl;

    server->startServer(50);

    return 0;
}