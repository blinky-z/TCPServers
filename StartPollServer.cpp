#include <iostream>
#include <set>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

using namespace std;

const unsigned short int PORT = 8080;
const unsigned int BACKLOG = 30;
const unsigned int POLL_SIZE = 2048;

int set_nonblock(int sockfd) {
    const int flags = fcntl(sockfd, F_GETFL, 0);
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
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

    struct pollfd workSockets[POLL_SIZE];
    workSockets[0].fd = masterSocket;
    workSockets[0].events = POLLIN;

    while (true) {
        unsigned int pollSockIndex = 1;
        for (int currentClientSocket : clientSockets) {
            workSockets[pollSockIndex].fd = currentClientSocket;
            workSockets[pollSockIndex].events = POLLIN;
            pollSockIndex++;
        }
        unsigned int workSocketsSize = 1 + clientSockets.size();

        poll(workSockets, workSocketsSize, -1);

        for (unsigned int currentWorkSocket = 0; currentWorkSocket < workSocketsSize; currentWorkSocket++) {
            if (workSockets[currentWorkSocket].revents & POLLIN) {
                if (!currentWorkSocket) {
                    int new_fd = accept(masterSocket, nullptr, nullptr);
                    set_nonblock(new_fd);
                    clientSockets.insert(new_fd);
                } else {
                    char readBuffer[1024];
                    memset(&readBuffer, '\0', sizeof(readBuffer));

                    int recvSize = recv(workSockets[currentWorkSocket].fd, &readBuffer, sizeof(readBuffer) - 1,
                                        MSG_NOSIGNAL);

                    if (recvSize == 0 && errno != EAGAIN) {
                        shutdown(workSockets[currentWorkSocket].fd, SHUT_RDWR);
                        close(workSockets[currentWorkSocket].fd);
                        clientSockets.erase(workSockets[currentWorkSocket].fd);
                    } else if (recvSize > 0) {
                        send(workSockets[currentWorkSocket].fd, &readBuffer, recvSize, MSG_NOSIGNAL);
                    } else {
                        cerr << "Error while receiving message. Error message: " << strerror(errno) << endl;
                        return -1;
                    }
                }
            }
        }
    }
}