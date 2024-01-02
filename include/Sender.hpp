#ifndef __SENDER_HPP__
#define __SENDER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include <mutex>

class Sender {
private:
    int sockfd_;
    std::mutex mutex_;
    std::vector<uint8_t> buffer_;

public:
    Sender() = delete;
    /*
     * Constructor.
     * @param sockfd: The sockfd to send messages on.
     */
    Sender(int sockfd);
    ~Sender();

    /*
     * Send a response.
     * @param response: The response to send.
     */
    bool send_response(Response &response);
};

#endif
