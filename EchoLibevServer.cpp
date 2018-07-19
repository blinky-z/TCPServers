#include <stdio.h>
#include <cstring>

#include <ev.h>
#include <netinet/in.h>

void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));
    ssize_t recv_size = recv(watcher->fd, buffer, sizeof(buffer) - 1, MSG_NOSIGNAL);

    if (recv_size < 0) {
        printf("Error reading data\n");
        return;
    } else if (recv_size == 0) {
        ev_io_stop(loop, watcher);
        delete watcher;
        return;
    } else {
        send(watcher->fd, buffer, recv_size, MSG_NOSIGNAL);
    }
}

void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
    int client_fd = accept(watcher->fd, nullptr, nullptr);

    struct ev_io* client_watcher = new ev_io;

    ev_io_init(client_watcher, read_cb, client_fd, EV_READ);
    ev_io_start(loop, client_watcher);
}

int main() {
    struct ev_loop* loop = ev_default_loop(0);

    int listenerSocket = socket(AF_INET, SOCK_STREAM, 0);

    int reuseAddr = 1;
    setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

    struct sockaddr_in listenerSockAddr;
    memset(&listenerSockAddr, 0, sizeof(listenerSockAddr));
    listenerSockAddr.sin_family = AF_INET;
    listenerSockAddr.sin_port = htons(8080);
    listenerSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenerSocket, (struct sockaddr*) &listenerSockAddr, sizeof(listenerSockAddr));
    listen(listenerSocket, SOMAXCONN);

    // accept watcher
    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, listenerSocket, EV_READ);
    ev_io_start(loop, &w_accept);

    ev_loop(loop, 0);

    return 0;
}
