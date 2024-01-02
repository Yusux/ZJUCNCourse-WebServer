#ifndef __RECEIVER_HPP__
#define __RECEIVER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include <mutex>
#include <sys/epoll.h>
#include <queue>
#include <atomic>

class Receiver {
private:
    std::mutex mutex_;
    int sockfd_;
    std::atomic<bool> running_;
    std::vector<uint8_t> buffer_;
    // It seems that message_queue_ is not needed
    // to be protected by another mutex.
    std::string remaining_;

public:
    Receiver() = delete;
    /*
     * Constructor.
     * @param sockfd: The sockfd to receive messages on.
     */
    Receiver(int sockfd);
    ~Receiver();

    /*
     * Close the receiver.
     */
    void close();

    /*
     * Receive a message.
     * @param request: The request to receive.
     * @return true if the message is received successfully, false otherwise.
     */
    bool get_request(Request &request);
};

#endif
