#include <iostream>
#include <set>
#include <unordered_map>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <poll.h>

using namespace std;

const unsigned short int PORT = 8080;
const unsigned int BACKLOG = 30;

struct ClientInfo {
    string address;
    string port;
};

int set_nonblock(int sockfd) {
    const int flags = fcntl(sockfd, F_GETFL, 0);
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int find_max_fd(int masterSocket, const set<int>& fdSockets) {
    if (!fdSockets.empty()) {
        return max(masterSocket, *fdSockets.rbegin());
    } else {
        return masterSocket;
    }
}

void sendToAll(const char* message, const set<int>& clients, int messageOwnerSock) {
    for (auto currentClientSock : clients) {
        if (currentClientSock != messageOwnerSock) {
            send(currentClientSock, message, strlen(message), MSG_NOSIGNAL);
        }
    }
}

void startServer(int masterSocket) {
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(PORT);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuseAddr = 1;
    setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

    if ((bind(masterSocket, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) == -1)) {
        throw std::runtime_error("[Server] Binding failed");
    }
    set_nonblock(masterSocket);

    if (listen(masterSocket, BACKLOG) == -1) {
        throw std::runtime_error("[Server] Listening failed");
    }

    printf("Waiting for connections on PORT %d\n", PORT);
}

int main() {
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    startServer(masterSocket);

    unordered_map<int, ClientInfo> clientInfo;
    set<int> clientSockets;

    while (true) {
        fd_set workSockets;
        FD_ZERO(&workSockets);

        FD_SET(masterSocket, &workSockets);
        for (auto currentSock : clientSockets) {
            FD_SET(currentSock, &workSockets);
        }

        int maxFd = find_max_fd(masterSocket, clientSockets);
        select(maxFd + 1, &workSockets, nullptr, nullptr, nullptr);

        for (auto currentSock = clientSockets.begin(); currentSock != clientSockets.end(); currentSock++) {
            if (FD_ISSET(*currentSock, &workSockets)) {
                char readBuffer[1024];
                memset(&readBuffer, '\0', sizeof readBuffer);
                int recvSize = recv(*currentSock, &readBuffer, sizeof readBuffer - 1, MSG_NOSIGNAL);

                if ((recvSize == 0) && (errno != EAGAIN)) {
                    string sendMessage = "Client with IP " + clientInfo[*currentSock].address +
                                         " and PORT " + clientInfo[*currentSock].port + " has been disconnected\n";

                    shutdown(*currentSock, SHUT_RDWR);
                    close(*currentSock);
                    clientSockets.erase(currentSock);

                    sendToAll(sendMessage.c_str(), clientSockets, -1);
                } else if (recvSize > 0) {
                    string sendMessage =
                            "Client's IP: " + clientInfo[*currentSock].address + " and PORT: " +
                            clientInfo[*currentSock].port + ". Client's message: " + string(readBuffer);

                    sendToAll(sendMessage.c_str(), clientSockets, *currentSock);
                } else {
                    cerr << "Error while receiving message from client with IP " + clientInfo[*currentSock].address +
                            " and PORT " + clientInfo[*currentSock].port +
                            ".Error message: " << strerror(errno) << endl;
                    return -1;
                }
            }
        }

        if (FD_ISSET(masterSocket, &workSockets)) {
            struct sockaddr_in clientAddr;
            socklen_t len = sizeof clientAddr;

            int newClientSock = accept(masterSocket, (struct sockaddr*) &clientAddr, &len);

            unsigned short int clientPort = htons(clientAddr.sin_port);
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof clientIP);

            clientInfo[newClientSock].address = string(clientIP);
            clientInfo[newClientSock].port = to_string(clientPort);

            set_nonblock(newClientSock);
            clientSockets.insert(newClientSock);

            string sendMessage = "Client with IP " + clientInfo[newClientSock].address + " and PORT " +
                                 clientInfo[newClientSock].port + " has been connected\n";

            sendToAll(sendMessage.c_str(), clientSockets, newClientSock);
        }
    }
}