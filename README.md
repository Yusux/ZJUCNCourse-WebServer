# Computer Network - Socket Programming

## Introduction

This is a simple to somewhat crude web server project for the course Computer Network. The project has implemented a simple web server, which can respond to the request from the client (like a browser).

The directory structure is as follows:

``` text
zjucn-webserver/
├── assets
│   ├── html
│   │   ├── noimg.html
│   │   └── test.html
│   ├── img
│   │   ├── favicon.ico
│   │   └── logo.jpg
│   └── txt
│       └── test.txt
├── include
│   ├── def.hpp
│   ├── Map.hpp
│   ├── Message.hpp
│   ├── Queue.hpp
│   ├── Receiver.hpp
│   └── Sender.hpp
├── lib
│   ├── Makefile
│   ├── Message.cpp
│   ├── Receiver.cpp
│   └── Sender.cpp
├── Makefile
├── Readme.md
└── src
    ├── include
    │   └── Server.hpp
    ├── Makefile
    └── server
        ├── main.cpp
        ├── Makefile
        └── Server.cpp
```

## Usage

This project is developed and tested on Arch Linux (6.6.2-arch1-1). MacOS and Windows are not supported since `epoll` is not supported on these platforms.

> ~~It seems epoll is not necessary for this project since the server can handle multiple clients by creating multiple threads. However, I still use epoll to implement the server since it is a good practice.~~
> Here I use `epoll` to poll the socket with some certain timeout in order to avoid the busy waiting while receiving the message non-blockingly.
> If you want to transfer the project to other platforms, you can try to ~~remove the epoll part (or~~ use `select` `poll` instead of `epoll` ~~)~~. It should work. :)
> Moreover, in this project, I use `future` in main function to wait for user's input. It can be replaced by `select` `poll` `epoll` to wait for both user's input and message queue.

### Compile

By using Makefile, you can compile the project by:

``` bash
make
```

This will make the server and client in the root directory with the name `server.out`.

### Server

``` bash
./server.out [host] [address] [port]    # Need to provide in sequence
```

> Graceful exit has been implemented in the server.

## Implementation

### Message, Request & Response

Message class is used to be the base class of Request and Response. Request and Response are used to encapsulate the request and response message.

``` cpp
class Message {
private:
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
}

class Request : public Message {
private:
    MethodTypes method_type_;
    std::string url_;
}

class Response : public Message {
private:
    StatusCodes status_code_;
}
```

### Sender & Receiver

Sender and Receiver class are used to encapsulate the sender and receiver methods.

### Server

What the server does is to receive the request from the client and send the response back to the client.
