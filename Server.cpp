//
// Created by zook on 01/11/18.
//

#include <arpa/inet.h>
#include "Server.h"

Server::Server() {}

bool Server::createSocketFD() {
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cout << "Creating socket failed";
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

int Server::acceptCon(sockaddr_in cli_addr, socklen_t cli_len) {
    // will write the client addr and len into the sent parameters
    new_socket = accept(sock_fd, (struct sockaddr *) &cli_addr, &cli_len);
    if (new_socket < 0) {
        cout << "Accepting connection failed";
        return -1;
    }
    cout << "server: got connection from " << inet_ntoa(cli_addr.sin_addr) << " port "
         << ntohs(cli_addr.sin_port) << std::endl;
    return new_socket;
}

bool Server::sendHeader(int socket, string data) {
    return send(socket, data.c_str(), strlen(data.c_str()), 0) > 0;
}

string Server::recieveData(int socket, int size, string fileName) {
    FILE *fp = fopen(fileName.c_str(), "w");
    char buffer[size];
    string data = "";
    if (recv(socket, buffer, sizeof(buffer), 0) > 0) {
        data = buffer;
        fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        fflush(fp);
    } else {
        cout << "Error in reciving message from server side!";
        return "";
    }
    fclose(fp);
    return data;
}

void Server::closeCon(int socket) {
    close(socket);
}
