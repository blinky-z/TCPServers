#include <iostream>
#include <cstring>
#include <set>
#include <algorithm>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace std;

const unsigned short int PORT = 8080;
const unsigned int BACKLOG = 30;
const unsigned int POLL_SIZE = 1024;

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

    set<int> clientSockets;

    while (true) {
        fd_set workSockets;
        FD_ZERO(&workSockets);

        FD_SET(masterSocket, &workSockets);
        for (auto currentSock = clientSockets.begin(); currentSock != clientSockets.end(); currentSock++) {
            FD_SET(*currentSock, &workSockets);
        }

        int maxFd = find_max_fd(masterSocket, clientSockets);
        select(maxFd + 1, &workSockets, nullptr, nullptr, nullptr);

        for (auto currentSock = clientSockets.begin(); currentSock != clientSockets.end(); currentSock++) {
            if (FD_ISSET(*currentSock, &workSockets)) {
                char readBuffer[1024];
                memset(&readBuffer, '\0', sizeof(readBuffer));
                int recvSize = recv(*currentSock, &readBuffer, sizeof(readBuffer) - 1, MSG_NOSIGNAL);

                if ((recvSize == 0) && (errno != EAGAIN)) {
                    shutdown(*currentSock, SHUT_RDWR);
                    close(*currentSock);
                    clientSockets.erase(currentSock);
                } else if (recvSize > 0) {
                    send(*currentSock, &readBuffer, recvSize, MSG_NOSIGNAL);
                } else {
                    cerr << "Error while receiving message. Error message: " << strerror(errno) << endl;
                    return -1;
                }
            }
        }

        if (FD_ISSET(masterSocket, &workSockets)) {
            int new_fd = accept(masterSocket, nullptr, nullptr);
            set_nonblock(new_fd);
            clientSockets.insert(new_fd);
        }
    }
}