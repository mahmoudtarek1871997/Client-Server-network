//
// Created by zook on 01/11/18.
//

#include <arpa/inet.h>
#include<pthread.h>
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

int Server::acceptCon() {
    int len = sizeof(struct sockaddr_in);
    // will write the client addr and len into the sent parameters
    new_socket = accept(sock_fd, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
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


void Server::startServer(int queueSize) {
    pthread_t threads[queueSize];
    int i = 0;
    while (i > 0) { // run forever
        struct sockaddr_in cli_add;
        socklen_t cli_len;
        serverArgs server_args;
        server_args.socket = acceptCon();
        server_args.server = this;
        if (pthread_create(&threads[i], NULL, socketThread, &server_args) != 0)
            printf("Failed to create thread\n");

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
    string filename = std::to_string(socket) + ".txt";
    Server *server = args->server;
    string data = server->recieveData(socket, 1500, filename);
    if (data[0] == 'P') { // post request
        /**
         *
         *
         *
         * */
    } else if (data[0] == 'G') { // get request
        /**
         *
         *
         *
         * */
    } else {
        cout << "Undefined request from client ! \n";
    }
    server->closeCon(socket);
    pthread_exit(NULL);

}

#define port 8080

int main() {
    std::cout << "Hello, Server! \n";
    Server *server = new Server();
    server->createSocketFD();
    cout << "socket created \n";
    bool res = server->bindServer(port);
    cout << "binding finished " << res << std::endl;

    server->listenToCon(50);
    cout << "Listening .." << std::endl;
    int soc = server->acceptCon();
    string data = server->recieveData(soc, 1500, std::to_string(soc) + ".txt");
    cout << "Data: " << data << std::endl;
    server->closeCon(soc);
    return 0;
}