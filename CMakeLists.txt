cmake_minimum_required(VERSION 3.10)
project(SocketLearn)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_executable(SocketLearn
#        CHOOSE ONLY ONE SERVER
#        ------------------------
#        blocking i/o server
#        StartBlockingServer.cpp
#        ------------------------
#        non-blocking i/o servers
#        select server
#        StartNonBlockingServer.cpp
#        poll server
#        StartPollServer.cpp
#        epoll server
#        StartEpollServer.cpp
#        chat server with using of select
#        ChatServer.cpp
#        echo server written with using of libevent library
        echoLibeventServer.cpp
        )

target_link_libraries(SocketLearn pthread)