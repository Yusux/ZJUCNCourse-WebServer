#include "Sender.hpp"
#include <sys/socket.h>

Sender::Sender(int sockfd) {
    sockfd_ = sockfd;
    buffer_.resize(MAX_BUFFER_SIZE);
}

Sender::~Sender() {}

bool Sender::send_response(Response &response) {
    std::unique_lock<std::mutex> lock(mutex_);
    ssize_t size = response.serialize(buffer_);
    size = send(sockfd_, reinterpret_cast<void *>(buffer_.data()), size, 0);
    return size != -1;
}
