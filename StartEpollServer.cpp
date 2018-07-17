#include <iostream>
#include <set>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

using namespace std;

const unsigned short int PORT = 8080;
const unsigned int BACKLOG = 30;
const unsigned int MAX_EVENTS = 2048;

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

    int EPoll = epoll_create1(0);
    struct epoll_event masterEvent;
    masterEvent.data.fd = masterSocket;
    masterEvent.events = EPOLLIN;

    epoll_ctl(EPoll, EPOLL_CTL_ADD, masterSocket, &masterEvent);

    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int N = epoll_wait(EPoll, events, MAX_EVENTS, -1);
        if (N == -1) {
            cerr << "Error occurred while checking for an I/O events. Error message: " << strerror(errno) << endl;
            return -1;
        }

        for (int eventIndex = 0; eventIndex < N; eventIndex++) {
            if (events[eventIndex].data.fd == masterSocket) {
                int newClientSock = accept(masterSocket, nullptr, nullptr);
                set_nonblock(newClientSock);

                struct epoll_event clientEvent;
                clientEvent.data.fd = newClientSock;
                clientEvent.events = EPOLLIN;

                epoll_ctl(EPoll, EPOLL_CTL_ADD, newClientSock, &clientEvent);
            } else {
                char readBuffer[1024];
                memset(&readBuffer, '\0', sizeof(readBuffer));

                int recvSize = recv(events[eventIndex].data.fd, &readBuffer, sizeof(readBuffer), MSG_NOSIGNAL);

                if ((recvSize == 0) & (errno != EAGAIN)) {
                    shutdown(events[eventIndex].data.fd, SHUT_RDWR);
                    close(events[eventIndex].data.fd);
                } else if (recvSize > 0) {
                    send(events[eventIndex].data.fd, &readBuffer, recvSize, MSG_NOSIGNAL);
                } else {
                    cerr << "Error while receiving message. Error message: " << strerror(errno) << endl;
                    return -1;
                }
            }
        }
    }
}