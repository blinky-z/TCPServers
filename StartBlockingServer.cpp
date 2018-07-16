#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int masterSocket;
const unsigned short int PORT = 8080;
const unsigned int backlog = 30;

void handle_client(int sockfd) {
    char readBuffer[1024];
    memset(&readBuffer, '\0', sizeof(readBuffer));

    while (read(sockfd, &readBuffer, sizeof(readBuffer) - 1) > 0) {
        write(sockfd, &readBuffer, sizeof(readBuffer));
        memset(&readBuffer, '\0', sizeof(readBuffer));
    }
}

void acceptConnections() {
    while (true) {
        int new_fd = accept(masterSocket, nullptr, nullptr);
        handle_client(new_fd);
    }
}

void startServer() {
    if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
        std::cout << "Server side listener socket has been created" << std::endl;
    } else {
        throw std::runtime_error("Unable to create server listener socket");
    }
    struct sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuseAddr = 1;
    setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

    if ((bind(masterSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1)) {
        throw std::runtime_error("[Server] Binding failed");
    }

    if ((listen(masterSocket, backlog)) == -1) {
        throw std::runtime_error("[Server] Listening failed");
    }

    printf("Waiting for connections on PORT %d\n", PORT);
}

int main() {
    startServer();
    acceptConnections();

    return 0;
}