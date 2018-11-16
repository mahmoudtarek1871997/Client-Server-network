//
// Created by yassmin on 31/10/18.
//

#include <vector>
#include <sstream>
#include <zconf.h>
#include "Client.h"

Client::Client() {}


vector<string> Client::split(string stringToBeSplitted, string delimeter) {
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

bool Client::conToserver(string hostName, int port) {
    struct sockaddr_in server_add;
    soc_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc_desc > 0) {
        server_add.sin_family = AF_INET;
        server_add.sin_port = htons(port);
        server_add.sin_addr = getHostIP(hostName);
        return connect(soc_desc, (struct sockaddr *) &server_add, sizeof(server_add)) == 0;
    }
    return false;
}


void Client::handleGET(string message, string fileName) {
    if (sendRequest(message)) {
        receiveResponse(1024, fileName);

    } else {
        cout << "Error while sending message: " << message << endl;
    }
}

void Client::handlePOST(string message) {
//    string header = "POST " + fileName + " HTTP/1.1\r\n\r\n"; // attach length header
//    cout << header << endl;
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (sendRequest(message)) {
//        if (recv(soc_desc, buffer, sizeof(buffer), 0) > 0) { // needs to parse ok 200 from server
//            if (strcmp(buffer, "OK") == 0) {
//                cout << "POST OK recieved from server\n";
//                cout << "sending file ....." << endl;
//                sendFile(fileName);
//
//            } else {
//                cout << buffer << "recieved from server!\n" << endl;
//            }
//        } else {
//            cout << "recieving failed!\n" << endl;
//        }
//    } else {
//        cout << "sending failed!\n" << endl;
    }
}

void Client::handleFIN() {
    cout << "FIN " << endl;
    if (sendRequest("FIN \r\n\r\n")) {
        cout << "FIN Sent" << endl;
        char buffer[7];
        string ack = "";
        memset(buffer, 0, sizeof(buffer));
        recv(soc_desc, buffer, sizeof(buffer), 0);
        ack = buffer;
        if (ack == "FINACK") {
            cout << "closing acknowledged" << endl;
            closeSocket();
        } else {
            cout << "Invalid ACK!, resend FIN." << endl;
            sendCloseSignal();
        }

    } else {
        cout << "Error while sending FIN message." << endl;
    }
}

void Client::handleRequest(string message) {

    vector<string> lines = split(message, "\r\n");
    // get the request type
    stringstream ss(lines[0]);
    vector<string> reqLine;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ' ');
        reqLine.push_back(substr);
    }
    string method = reqLine[0];
    string fileName = reqLine[1];

    if (method == "GET") {
        handleGET(message, fileName);
    } else if (method == "POST") {
        handlePOST(message);
    } else if (method == "FIN") {
        handleFIN();
    } else {
        cout << "invalid request!" << endl;
    }
}

bool Client::sendRequest(string request) {
    int size = send(soc_desc, request.c_str(), strlen(request.c_str()), 0);
    if (size > 0) {
        cout << "header sent, size: " << size << endl;
        return true;
    }
    return false;
}

string Client::receiveResponse(int size, string fileName) {
    char buffer[size];
    string response = "";
    ssize_t recvSize;
    recvSize = recv(soc_desc, buffer, sizeof(buffer), 0);
    int i = 0;
    while (!(buffer[i] == '\r' && buffer[i + 1] == '\n') && i < recvSize) {
        response += buffer[i];
        i++;
    }
    cout << "server response: " << response << endl;
    response = response.substr(9, 3); // get response number
    if (response == "200") { //OK
        int len = getContentLen(buffer, i + 2, recvSize);
        // move i to the start position of the data
        while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n'))
            i++;
        i += 4;
        char data[1024];
        int j = 0;
        cout << "recv size  " << recvSize << "   len  " << len << endl;
        FILE *fp = fopen(fileName.c_str(), "w");
        while (len > 0) { // need piplining ////////////////////////////////////////////////////////////////
            while (i < recvSize) {
                data[j++] = buffer[i++];
                len--;
                if (len == 0 && i < recvSize)
                    break;
            }
            fwrite(data, sizeof(char), j, fp);
            fflush(fp);
            if (len > 0) { // there are more data
                i = 0, j = 0;
                memset(buffer, 0, sizeof(buffer));
                recvSize = recv(soc_desc, buffer, sizeof(buffer), 0);
            }
        }

    }


    return "";

}

struct in_addr Client::getHostIP(string hostName) {
    struct hostent *host;
    struct in_addr **in_list;

    if ((host = gethostbyname(hostName.c_str())) != NULL) {
        in_list = (struct in_addr **) host->h_addr_list;
        return **in_list;
    }
}

void Client::sendFile(string fileName) {
    char buff[1024];
    memset(buff, 0, sizeof(buff));
    FILE *fp = fopen(fileName.c_str(), "r");
    int read = 0;
    while ((read = fread(buff, 1, sizeof(buff), fp)) > 0) {
        send(soc_desc, buff, read, 0);
        memset(buff, 0, sizeof(buff));
    }

    fclose(fp);
    cout << "Sending finished successfully" << endl;
}

void Client::sendCloseSignal() {
    string data = "FIN";
    handleRequest(data);
}

void Client::closeSocket() {
    close(soc_desc);
}

int Client::getContentLen(char *buffer, int startIndex, int recvSize) {
    string response = "";
    int i = startIndex;
    while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') &&
           i < recvSize)  // get the remain of the response message
        response += buffer[i++];
    vector<string> headers = split(response, "\r\n");
    for (string str: headers) {
        if (str.substr(0, 14).compare("Content-Length") == 0) { // if Content-Length Header
            stringstream lenStream(str.substr(16, str.size() - 15));
            int len = 0;
            lenStream >> len;
            return len;
        } // else ignore other headers
    }
    // if content length not found return -1
    return -1;

}


int main(int argc, char *argv[]) {

    Client c;
    string data = "GET test.html HTTP/1.1\r\n\r\n";
    if (!c.conToserver("localhost", 8080))
        cout << "error while connecting" << endl;
    c.handleRequest(data);
//    c.handleRequest(data);
//    c.sendCloseSignal();

    return 0;
}