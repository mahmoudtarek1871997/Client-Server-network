//
// Created by yassmin on 31/10/18.
//

#include <vector>
#include <sstream>
#include <zconf.h>
#include "Client.h"

Client::Client() {}

bool Client::conToserver(string hostName, int port) {
    soc_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc_desc > 0) {
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr = getHostIP(hostName);
        int con = connect(soc_desc, (struct sockaddr *) &server, sizeof(server));
        return con > 0;
    }
    return false;
}

void Client::handleRequest(string req) {
    stringstream ss(req);
    vector<string> result;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ' ');
        result.push_back(substr);
    }
    string method = result[0];
    string fileName = result[1];

    if (method == "GET") {
        cout << "GET " + fileName + " HTTP/1.1\r\n\r\n";
        if (sendHeader("GET " + fileName + " HTTP/1.1\r\n\r\n")) {
            cout << recieveData(1024, fileName);
        } else {
            cout << "Error while sending Header." << endl;
        }

    } else {
        cout << "POST " + fileName + " HTTP/1.1" << endl;
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        if (send(soc_desc, req.c_str(), strlen(req.c_str()), 0) > 0) {
            if (recv(soc_desc, buffer, sizeof(buffer), 0) > 0) {
                if (strcmp(buffer, "OK") == 0) {
                    cout << "POST OK recieved from server\n";
                    cout << "sending file ....." << endl;
                    sendFile(fileName);

                } else {
                    cout << buffer << "recieved from server!\n" << endl;
                }
            } else {
                cout << "recieving failed!\n" << endl;
            }
        } else {
            cout << "sending failed!\n" << endl;
        }
    }
}

bool Client::sendHeader(string data) {
    int size = send(soc_desc, data.c_str(), strlen(data.c_str()), 0);
    if (size > 0) {
        cout << "header sent, size: " << size << endl;
        return true;
    }
    return false;
}

string Client::recieveData(int size, string fileName) {   /////////////change can't read the hole file
    FILE *fp = fopen(fileName.c_str(), "w");
    char buffer[size];
    string data = "";
    ssize_t recvSize;
    do {
        memset(buffer, 0, sizeof(buffer));
        recvSize = recv(soc_desc, buffer, sizeof(buffer), 0);
        if (recvSize > 0) {
            //  cout << "recvSize " << recvSize << endl;
            data += buffer;
            fwrite(buffer, sizeof(char), recvSize, fp);
            fflush(fp);
        }
    } while (recvSize > 0);
    fclose(fp);
    return data;

}

struct in_addr Client::getHostIP(string hostName) {
    struct hostent *host;
    struct in_addr **in_list;

    if ((host = gethostbyname(hostName.c_str())) != NULL) {
        in_list = (struct in_addr **) host->h_addr_list;
        // cout << hostName << ": " << inet_ntoa(**in_list) << "\n";
        return **in_list;
    }
}

void Client::sendFile(string fileName) {
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    FILE *fp = fopen(fileName.c_str(), "r");
    int read = 0;
    while ((read = fread(buff, 1, sizeof(buff), fp)) > 0) {
        // cout << "buff" << buff << "\n";
        send(soc_desc, buff, read, 0);
        memset(buff, 0, sizeof(buff));
    }

    fclose(fp);
    cout << "Sending finished successfully" << endl;
}

void Client::closeSocket() {
    close(soc_desc);
}

int main(int argc, char *argv[]) {

    Client c;
    string data = "POST test.html HTTP/1.1";
    c.conToserver("localhost", 8080);
    c.handleRequest(data);
    c.closeSocket();

    return 0;
}