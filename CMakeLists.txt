cmake_minimum_required(VERSION 3.10)
project(SocketLearn)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_executable(SocketLearn
#        blocking i/o server
        StartBlockServer.cpp
#        non-blocking i/o servers
#        select server
        StartNonBlockServer.cpp
#        poll server
        StartPollServer.cpp
#        epoll server
        StartEpollServer.cpp
        )

target_link_libraries(SocketLearn pthread)