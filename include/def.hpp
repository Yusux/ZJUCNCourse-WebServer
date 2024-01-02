#ifndef __DEF_HPP__
#define __DEF_HPP__

#define MAX_BUFFER_SIZE 65536
#define MAX_CLIENT_NUM 255
#define MAX_EPOLL_EVENTS 1
#define TIMEOUT 200

#define SERVER_ADDR INADDR_ANY
#define SERVER_PORT 2024
#define USERNAME "username"
#define PASSWORD "password"

typedef unsigned char uint8_t;

#define cast_sockaddr_in(addr) reinterpret_cast<sockaddr *>(&(addr))

#endif
