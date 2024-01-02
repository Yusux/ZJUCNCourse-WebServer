#include "Receiver.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

Receiver::Receiver(int sockfd) : sockfd_(sockfd), running_(true) {
    buffer_.resize(MAX_BUFFER_SIZE);
}

Receiver::~Receiver() {}

void Receiver::close() {
    running_ = false;
}

bool Receiver::get_request(Request &request) {
    std::unique_lock<std::mutex> lock(mutex_);
    // use epoll_wait to wait for the socket to be readable
    int epollfd_ = epoll_create(MAX_EPOLL_EVENTS), nfds, content_length;
    // add the socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sockfd_;
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd_, &event) == -1) {
        std::string error_message = "epoll_ctl error: fd = " + std::to_string(sockfd_) +
                                    ", errno = " + std::to_string(errno);
        perror(error_message.c_str());
        return false;
    }

    // prepare variables
    std::vector<struct epoll_event> events_(MAX_EPOLL_EVENTS);
    MethodTypes method_type = MethodTypes::UNKNOWN;
    std::string method, url, version, body;
    std::unordered_map<std::string, std::string> headers;

    while (true) {
        while ((nfds = epoll_wait(epollfd_, events_.data(), MAX_EPOLL_EVENTS, TIMEOUT)) == 0) {
            // if closed, return 0
            if (!running_) {
                return false;
            }
        }

        // if epoll_wait returns 0, it means that the timeout expires
        // if epoll_wait returns -1, it means that an error occurs
        if (nfds == 0) {
            continue;
        }
        if (nfds == -1) {
            std::string error_message = "epoll_wait error: nfds = " + std::to_string(nfds) +
                                        ", errno = " + std::to_string(errno);
            perror(error_message.c_str());
            return false;
        }

        // receive the message
        ssize_t size = recv(sockfd_, reinterpret_cast<void *>(buffer_.data()), MAX_BUFFER_SIZE, 0);
        if (size == -1) {
            std::string error_message = "recv error: size = " + std::to_string(size) +
                                        ", errno = " + std::to_string(errno);
            perror(error_message.c_str());
            return false;
        }
        if (size == 0) {
            // if the peer has performed an orderly shutdown
            return false;
        }

        // process the http request
        remaining_ += std::string(buffer_.begin(), buffer_.begin() + size);
        if (
            method_type == MethodTypes::UNKNOWN &&
            remaining_.find("\r\n\r\n") != std::string::npos
        ) {
            // parse the headers
            int pos = remaining_.find("\r\n\r\n");
            std::string header_string = remaining_.substr(0, pos);
            remaining_ = remaining_.substr(pos + 4);
            std::istringstream iss(header_string);
            iss >> method >> url >> version >> std::ws;
            std::string line;
            while (std::getline(iss, line)) {
                if (line == "") {
                    break;
                }
                std::istringstream iss2(line);
                std::string key, value;
                iss2 >> key >> value;
                headers.insert(std::make_pair(key.substr(0, key.size() - 1), value));
            }
            if (method == "GET") {
                method_type = MethodTypes::GET;
                break;
            } else if (method == "POST") {
                method_type = MethodTypes::POST;
                if (headers.find("Content-Length") != headers.end()) {
                    content_length = std::stoi(headers["Content-Length"]);
                } else {
                    content_length = 0;
                }
            } else {
                break;
            }
        }
        if (method_type == MethodTypes::POST && remaining_.size() >= content_length) {
            body = remaining_.substr(0, content_length);
            remaining_ = remaining_.substr(content_length);
            break;
        }
    }

    // construct the message
    request = Request(method_type, url, version, body, headers);
    return true;
}
