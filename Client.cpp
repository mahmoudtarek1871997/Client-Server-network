//
// Created by yassmin on 31/10/18.
//

#include <vector>
#include <sstream>
#include <fstream>
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
        fileNames.push(fileName);
    } else {
        cout << "Error while sending message: " << message << endl;
    }
}

void Client::handlePOST(string message, string fileName, int len) {
    if (sendRequest(message)) {
        postFileName = fileName;
        fileLen = len; // save file length to use it while sending file
        cout << "post sent" << endl;
        pthread_join(recvThread, NULL);
        cout << "post finished \n\n" << endl;
    } else {
        cout << "sending: " << message << " :failed!\n" << endl;
    }
}

void Client::handleFIN(string message) {
    cout << message << endl;
    if (sendRequest(message)) {
        cout << "FIN Sent" << endl;
        char buffer[10];
        string ack = "";
        memset(buffer, 0, sizeof(buffer));
        int recvSize = recv(soc_desc, buffer, sizeof(buffer), 0);
        ack = buffer;
        if (ack == "FINACK\r\n\r\n") {
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

int Client::handleRequest(string message) {

    vector<string> lines = split(message, "\\r\\n");
    // get the request type
    stringstream ss(lines[0]);
    vector<string> reqLine;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ' ');
        reqLine.push_back(substr);
    }
    string method = reqLine[0];
    string fileName = "";
    if (method != "FIN")
        fileName = reqLine[1];

    if (method == "GET") {
        handleGET(message, fileName);
    } else if (method == "POST") {
        vector<string> cl = split(lines[1], ": ");
        stringstream ss(cl[1]);
        int len = 0;
        ss >> len;
        handlePOST(message, fileName, len);
        return 1;
    } else if (method == "FIN") {
        handleFIN(message);
    } else {
        cout << "invalid request!" << endl;
        return -1;
    }
    return 0;
}

bool Client::sendRequest(string request) {
    int size = send(soc_desc, request.c_str(), strlen(request.c_str()), 0);
    if (size > 0) {
        cout << "request sent: \n" << request << endl;
        return true;
    }
    return false;
}

struct in_addr Client::getHostIP(string hostName) {
    struct hostent *host;
    struct in_addr **in_list;

    if ((host = gethostbyname(hostName.c_str())) != NULL) {
        in_list = (struct in_addr **) host->h_addr_list;
        return **in_list;
    }
}

void Client::sendFile(string fileName, int len) {
    char buff[len];
    memset(buff, 0, sizeof(buff));
    FILE *fp = fopen(fileName.c_str(), "r");
    int read = 0;
    cout << fileName << endl;
    while ((read = fread(buff, 1, sizeof(buff), fp)) > 0) {
        send(soc_desc, buff, read, 0);
        memset(buff, 0, sizeof(buff));
    }

    fclose(fp);
    cout << "Sending finished successfully" << endl;
}

void Client::sendCloseSignal() {
    string data = "FIN\r\n\r\n";
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
    vector<string> headers = split(response, "\\r\\n");
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

void *receive(void *arg) {
    Client *c = (Client *) arg;
    while (1) {
        char buffer[1024];
        string response = "";
        ssize_t recvSize;
        recvSize = recv(c->soc_desc, buffer, sizeof(buffer), 0);
        int i = 0;

        while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') &&
               i < recvSize) {
            response += buffer[i];
            i++;
        }
        if (response == "HTTP/1.0 200 OK") { // post ack
            string fileName = c->postFileName;
            cout << "POST OK recieved from server\n";
            cout << "sending file ....." << endl;
            c->sendFile(fileName, c->fileLen);
            break; // exit receiving thread
        } else { // get ack with file
            string fileName = (string) c->fileNames.front();
            c->fileNames.pop();
            response = "";
            i = 0;
            while (!(buffer[i] == '\r' && buffer[i + 1] == '\n') && i < recvSize) {
                response += buffer[i];
                i++;
            }
            cout << "server response: " << response << endl;

            if (response.size() > 0)
                response = response.substr(9, 3); // get response number
            if (response == "200") { //OK
                int len = c->getContentLen(buffer, i + 2, recvSize);
                // move i to the start position of the data
                while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n'))
                    i++;
                i += 4;
                cout << "file: " << fileName << " - len: " << len << endl;
                char data[1024];
                int j = 0;
                FILE *fp = fopen(fileName.c_str(), "w");
                while (len > 0) {
                    while (i < recvSize) {
                        data[j++] = buffer[i++];
                        len--;
                        if (len == 0 && i < recvSize){
                            break;
                        }
                    }
                    fwrite(data, sizeof(char), j, fp);
                    fflush(fp);
                    if (len > 0) { // there are more data
                        i = 0, j = 0;
                        memset(buffer, 0, sizeof(buffer));
                        recvSize = recv(c->soc_desc, buffer, sizeof(buffer), 0);
                    }
                }
                if (i < recvSize) { // there are more in the buffer
                    bool isPost = handleRemainder(buffer, i, recvSize, c);
                    if (isPost)
                        break;
                }

            } else { // 404 not found
                if (i < recvSize) { // there are more in the buffer
                    bool isPost = handleRemainder(buffer, i + 4, recvSize, c);
                    if (isPost)
                        break;
                }
            }
        }
    }

}

/**
 * return true if a post response was handled - then the recv thread will exit
 * */
bool handleRemainder(char *buffer, int i, int recvSize, Client *c) {

    int iTemp = i;
    string response = "";
    int counter = 0;
    while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') &&
           i < recvSize) {
        response += buffer[i];
        i++;
        counter++;
    }
    while (counter < 15) { // for example just "HTT" is the remainder of the buffer,
        // we need to get the remainder of the response
        memset(buffer, 0, sizeof(*buffer));
        recvSize = recv(c->soc_desc, buffer, sizeof(*buffer), 0);
        i = 0;
        while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') &&
               i < recvSize) {
            response += buffer[i];
            i++;
            counter++;
        }
        if (counter < 15) // if still < 15, sleep for 500 ms to ensure all new data is arrived
            sleep(.5);
    }

    if (response == "HTTP/1.0 200 OK") { // post ack
        string fileName = c->postFileName;
        cout << "POST OK recieved from server\n";
        cout << "sending file ....." << endl;
        c->sendFile(fileName, c->fileLen);
        return true;
    } else { // get ack with file

        string fileName = (string) c->fileNames.front();
        c->fileNames.pop();
        response = "";
        i = iTemp;
        while (!(buffer[i] == '\r' && buffer[i + 1] == '\n') && i < recvSize) {
            response += buffer[i];
            i++;
        }
        cout << "server response: " << response << endl;
        if (response.size() > 0)
            response = response.substr(9, 3); // get response number
        if (response == "200") { //OK
            int len = c->getContentLen(buffer, i + 2, recvSize);
            // move i to the start position of the data
            while (!(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n'))
                i++;
            i += 4;
            cout << "file: " << fileName << " - len: " << len << endl;
            char data[1024];
            int j = 0;
            FILE *fp = fopen(fileName.c_str(), "w");
            while (len > 0) {
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
                    memset(buffer, 0, sizeof(*buffer));
                    recvSize = recv(c->soc_desc, buffer, sizeof(*buffer), 0);
                }
            }
            if (i < recvSize) {
                bool isPost = handleRemainder(buffer, i, recvSize, c);
                if (isPost)
                    return true;
            }
        } else { // not found
            cout << "file: " << fileName << " not found!" << endl;
            if (i < recvSize) {
                bool isPost = handleRemainder(buffer, i + 4, recvSize, c);// handle after not found
                if (isPost)
                    return true;
            }
        }
    }
    return false;
}


int main(int argc, char *argv[]) {

    Client c;
    if (!c.conToserver("localhost", 8080)) {
        cout << "error while connecting" << endl;
        exit(1);
    }

    int res = 0;

    pthread_create(&c.recvThread, NULL, receive, &c);

    fstream file("commands.txt");
    string command;
    while (getline(file, command))
    {
        res = c.handleRequest(command);
        if (res == 1) // if post, recreate receive thread
            pthread_create(&c.recvThread, NULL, receive, &c);

    }
    file.close();
    pthread_join(c.recvThread, NULL);
    c.sendCloseSignal();

    /*   int res = 0;
       pthread_create(&c.recvThread, NULL, receive, &c);
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "POST test2.txt HTTP/1.1\r\nContent-Length: 1083\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "GET test3.txt HTTP/1.1\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "GET test4.txt HTTP/1.1\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "GET test5.txt HTTP/1.1\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "GET test6.txt HTTP/1.1\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

       data = "GET test7.txt HTTP/1.1\r\n\r\n";
       res = c.handleRequest(data);
       if (res == 1) // if post, recreate receive thread
           pthread_create(&c.recvThread, NULL, receive, &c);

   //    c.sendCloseSignal();
       pthread_join(c.recvThread, NULL);*/

    return 0;
}