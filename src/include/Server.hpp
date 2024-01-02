#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "def.hpp"
#include "Message.hpp"
#include "Receiver.hpp"
#include "Sender.hpp"
#include "Map.hpp"
#include "Queue.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>

#define get_client_addr(client_id, lock)\
    std::string(\
        inet_ntoa(clientinfo_list_->at((client_id), (lock))->get_addr().sin_addr)\
    ) +\
    ":" +\
    std::to_string(\
        ntohs(clientinfo_list_->at((client_id), (lock))->get_addr().sin_port)\
    )

enum class FileTypes {
    HTML,
    JPG,
    ICO,
    TXT
};

struct File {
    FileTypes type;
    std::string path;
    bool is_post = false;
};

std::string get_file_type(FileTypes type);

class ClientInfo {
private:
    int sockfd_;
    sockaddr_in addr_;
    uint8_t client_id_;
    std::unique_ptr<Sender> sender_;
    std::unique_ptr<Receiver> receiver_;


public:
    ClientInfo(
        sockaddr_in addr,
        int sockfd,
        uint8_t id,
        Sender *sender,
        Receiver *receiver
    );
    ~ClientInfo();

    sockaddr_in get_addr();
    Sender *get_sender();
    Receiver *get_receiver();
};

class Server {
private:
    int sockfd_;
    sockaddr_in server_addr_;
    std::atomic_bool running_;
    const std::unordered_map<std::string, File> route_;
    // Use Map/Queue with mutex for thread safety.
    std::unique_ptr<Map<uint8_t, std::unique_ptr<ClientInfo> > > clientinfo_list_;
    std::unique_ptr<Map<uint8_t, std::unique_ptr<std::thread> > > client_recv_list_;
    std::unique_ptr<Queue<std::string> > output_queue_;

    /*
     * Wait for clients to connect.
     */
    void wait_for_client();

    /*
     * Keep receiving messages from the client.
     * @param client_id The id of the client.
     */
    void receive_from_client(uint8_t client_id);

    /*
     * Join the threads.
     */
    void join_threads();

public:
    /*
     * Connect to the server.
     * @param name The name of the client.
     */
    Server(
        std::string name,
        in_addr_t addr,
        int port,
        std::unordered_map<std::string, File> route
    );
    ~Server();

    /*
     * Run the server.
     * Keeping accepting connections from clients.
     * Once a client is connected, a thread will be created for the client
     * to receive messages from the client and do the corresponding actions.
     */
    void run();

    /*
     * Stop the server.
     */
    void stop();

    /*
     * Print the message queue.
     * @return Whether the printing is successful.
     */
    bool output_message();
};

#endif
