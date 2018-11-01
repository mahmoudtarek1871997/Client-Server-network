#include <iostream>
#include "Client.h"
#include "Server.h"

#define port 8080

int main() {
    std::cout << "Hello, World!" << std::endl;
    Client *client = new Client();
    client->conToserver("localhost", port);
    Server *server = new Server();
    server->createSocketFD();
    server->bindServer(port);
    server->listenToCon(5);
    struct sockaddr_in cli_add;
    socklen_t clilen;
    int soc_fd = server->acceptCon(cli_add, clilen);
    client->sendHeader("Hello Server !");
    if (soc_fd != -1) {
        string data = server->recieveData(soc_fd, 1500, "test.txt");
        cout << data << std::endl;
    }
    cout << "finished !" << std::endl;
    return 0;
}