#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cerrno>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>

void echo_read_cb(struct bufferevent* bev, void* ctx) {
    struct evbuffer* input = bufferevent_get_input(bev);
    struct evbuffer* output = bufferevent_get_output(bev);

    size_t length = evbuffer_get_length(input);
    char* data = new char[length + 1];
    evbuffer_copyout(input, data, length);
    data[length] = '\0';
    printf("data: %s\n", data);

    evbuffer_add_buffer(output, input);
    delete[](data);
}

void echo_event_cb(struct bufferevent* bev, short events, void* ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Error");
        bufferevent_free(bev);
    }

    if (events & BEV_EVENT_EOF) {
        bufferevent_free(bev);
    }
}

void accept_conn_cb(
        struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* address, int socklen, void* ctx) {
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void accept_error_cb(struct evconnlistener* listener, void* ctx) {
    struct event_base* base = evconnlistener_get_base(listener);

    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Error = %d = \"%s\"\n", err, evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

int main(int argc, char* argv[]) {
    const char** methods = event_get_supported_methods();
    printf("Starting Libevent %s.  Available methods are:\n", event_get_version());
    for (int i = 0; methods[i] != NULL; ++i) {
        printf("    %s\n", methods[i]);
    }
    std::cout << std::endl;

    struct event_base* base;

    unsigned short int port = std::stoi(argv[1]);
    std::string backendMethod = argv[2];
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct event_config* cfg = event_config_new();

    if (backendMethod == "select") {
        event_config_avoid_method(cfg, "poll");
        event_config_avoid_method(cfg, "epoll");
    }
    if (backendMethod == "poll") {
        event_config_avoid_method(cfg, "select");
        event_config_avoid_method(cfg, "epoll");
    }
    if (backendMethod == "epoll") {
        event_config_avoid_method(cfg, "select");
        event_config_avoid_method(cfg, "poll");
    }

    base = event_base_new_with_config(cfg);
    event_config_free(cfg);

    if (!base) {
        std::cerr << "EvListener with using of " << backendMethod << " backend method creation failure" << std::endl;
        return -1;
    } else {
        std::cout << "EvListener with using of " << backendMethod << " backend method has been created" << std::endl;
    }

    struct evconnlistener* listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                                              (struct sockaddr*) &sockAddr, sizeof(sockAddr));

    std::cout << "EvListener listens on PORT " << port << std::endl;

    evconnlistener_set_error_cb(listener, accept_error_cb);

    event_base_dispatch(base);

    return 0;
}