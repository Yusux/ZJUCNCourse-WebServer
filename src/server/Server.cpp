#include "Server.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstring>
#include <netinet/tcp.h>

std::string get_file_type(FileTypes type) {
    switch (type) {
        case FileTypes::HTML:
            return "text/html";
        case FileTypes::JPG:
            return "image/jpeg";
        case FileTypes::ICO:
            return "image/x-icon";
        case FileTypes::TXT:
            return "text/plain";
        default:
            throw std::invalid_argument("Invalid file type");
    }
}

ClientInfo::ClientInfo(
    sockaddr_in addr,
    int sockfd,
    uint8_t id,
    Sender *sender,
    Receiver *receiver
) : sockfd_(sockfd), addr_(addr), client_id_(id) {
    sender_ = std::unique_ptr<Sender>(sender);
    receiver_ = std::unique_ptr<Receiver>(receiver);
}

ClientInfo::~ClientInfo() {
    // Close the socket.
    close(sockfd_);
}

sockaddr_in ClientInfo::get_addr() {
    return addr_;
}

Sender *ClientInfo::get_sender() {
    return sender_.get();
}

Receiver *ClientInfo::get_receiver() {
    return receiver_.get();
}

Server::Server(
    std::string name,
    in_addr_t addr,
    int port,
    std::unordered_map<std::string, File> route
) : running_(true), route_(route) {
    // Prepare the server_addr_.
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = addr;

    // Create a socket.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::string error_msg = "Server Init failed: failed to create a socket. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Set the socket to be reusable.
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(sockfd);
        std::string error_msg = "Server Init failed: failed to set the socket to be reusable. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Bind the socket to the server address and port.
    if (bind(sockfd, cast_sockaddr_in(server_addr_), sizeof(server_addr_)) < 0) {
        close(sockfd);
        std::string error_msg = "Server Init failed: failed to bind the socket to the server address and port. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Listen for connections for maximum MAX_CLIENT_NUM clients.
    listen(sockfd, MAX_CLIENT_NUM);

    // Save the socket.
    sockfd_ = sockfd;

    // Create the lists.
    clientinfo_list_ = std::unique_ptr<Map<uint8_t, std::unique_ptr<ClientInfo> > >(
        new Map<uint8_t, std::unique_ptr<ClientInfo> >()
    );
    client_recv_list_ = std::unique_ptr<Map<uint8_t, std::unique_ptr<std::thread> > >(
        new Map<uint8_t, std::unique_ptr<std::thread> >()
    );
    output_queue_ = std::unique_ptr<Queue<std::string> >(
        new Queue<std::string>()
    );
}

Server::~Server() {
    // Join all the client threads.
    output_queue_->push("[INFO] Releasing the threads.");
    output_message();
    join_threads();
    output_queue_->push("[INFO] Released the threads.");
    output_message();

    // Close all the client connections.
    // Get shared_mutex for clientinfo_list_.
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());

    // Close the socket.
    close(sockfd_);

    // Output the remaining messages.
    output_queue_->push("[INFO] Released the server.");
    output_message();
}

void Server::wait_for_client() {
    // Accept a connection for client.
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd_, cast_sockaddr_in(client_addr), &client_addr_len);
    if (!running_) {
        // if the server is not running, close the socket and return.
        if (client_sockfd >= 0) {
            close(client_sockfd);
        }
        return;
    }
    if (client_sockfd < 0) {
        std::string error_msg = "Server Wait For Client failed: failed to accept a connection. errno: " +
                                std::to_string(errno) + " " + strerror(errno);
        throw std::runtime_error(error_msg);
    }

    // Create a client info.
    // Find a valid client id.
    uint8_t id = 1;
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    while (clientinfo_list_->check_exist(id, clientinfo_list_lock)) {
        id++;
        if (id == 0) {
            close(client_sockfd);
            throw std::runtime_error("Server Wait For Client failed: no free client id.");
        }
    }

    // Create a client info.
    Receiver *receiver = new Receiver(client_sockfd);
    Sender *sender = new Sender(client_sockfd);
    std::unique_ptr<ClientInfo> client_info = std::make_unique<ClientInfo>(
        client_addr,
        client_sockfd,
        id,
        sender,
        receiver
    );
    clientinfo_list_->insert_or_assign(id, std::move(client_info), clientinfo_list_lock);
    // Create threads for the client.
    std::unique_ptr<std::thread> recv_thread = std::make_unique<std::thread>(
        &Server::receive_from_client,
        this,
        id
    );
    std::unique_lock<std::mutex> client_recv_list_lock(client_recv_list_->get_mutex());
    if (client_recv_list_->check_exist(id, client_recv_list_lock)) {
        client_recv_list_->at(id, client_recv_list_lock)->join();
        client_recv_list_->at(id, client_recv_list_lock).release();
    }
    client_recv_list_->insert_or_assign(id, std::move(recv_thread), client_recv_list_lock);
}

void Server::receive_from_client(uint8_t client_id) {
    // Check if the client id is valid.
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    if (!clientinfo_list_->check_exist(client_id, clientinfo_list_lock)) {
        throw std::runtime_error("Server Receive From Client failed: invalid client id.");
    }

    Sender *sender = clientinfo_list_->at(client_id, clientinfo_list_lock)->get_sender();
    Receiver *receiver = clientinfo_list_->at(client_id, clientinfo_list_lock)->get_receiver();

    // delete the unique_lock
    clientinfo_list_lock.unlock();

    Request request;
    if (receiver->get_request(request) && running_) {
        std::unique_lock<std::mutex> lock(clientinfo_list_->get_mutex());

        // Print the message.
        output_queue_->push(
            "[INFO] Received request from " +
            get_client_addr(client_id, lock)
        );

        // check the type of the request.
        // Prepare the response.
        StatusCodes status_code;
        std::unordered_map<std::string, std::string> headers;
        std::string body = "";
        if (request.get_method_type() == MethodTypes::GET) {
            // Log the request.
            output_queue_->push(
                "[INFO] GET " +
                request.get_url() +
                " " +
                request.get_version() +
                " from " +
                get_client_addr(client_id, lock)
            );

            // Check if the url is valid.
            std::string url = request.get_url();
            if (route_.find(url) == route_.end()) {
                // If the url is not found, return 404.
                status_code = StatusCodes::NOT_FOUND;

                // Prepare the 404 response body.
                body = "<html><body><h1>404 Not Found</h1></body></html>";

                // Prepare the 404 response headers.
                headers["Content-Type"] = "text/html";
                headers["Content-Length"] = std::to_string(body.length());
            } else {
                // Get the file and prepare the response body.
                File file = route_.at(url);
                // Open the file.
                std::ifstream file_stream(file.path, std::ios::in | std::ios::binary);
                if (file_stream.is_open()) {
                    // If the url is found, and the file is opened, return 200.
                    status_code = StatusCodes::OK;

                    // Read the file.
                    while (file_stream.eof() == false) {
                        std::string line;
                        std::getline(file_stream, line);
                        body += line + "\n";
                    }

                    // Prepare the 200 response headers.
                    headers["Content-Type"] = get_file_type(file.type);
                    headers["Content-Length"] = std::to_string(body.length());
                } else {
                    // Return 500 if the file cannot be opened.
                    status_code = StatusCodes::INTERNAL_SERVER_ERROR;

                    // Prepare the 500 response body.
                    body = "<html><body><h1>500 Internal Server Error</h1></body></html>";

                    // Prepare the 500 response headers.
                    headers["Content-Type"] = "text/html";
                    headers["Content-Length"] = std::to_string(body.length());
                }
            }
        } else if (request.get_method_type() == MethodTypes::POST) {
            // Log the request.
            output_queue_->push(
                "[INFO] POST " +
                request.get_url() +
                " " +
                request.get_version() +
                " from " +
                get_client_addr(client_id, lock)
            );

            // Check if the url is valid.
            std::string url = request.get_url();
            if (url != "/dopost") {
                // If the url is not found, return 404.
                status_code = StatusCodes::NOT_FOUND;

                // Prepare the 404 response body.
                body = "<html><body><h1>404 Not Found</h1></body></html>";

                // Prepare the 404 response headers.
                headers["Content-Type"] = "text/html";
                headers["Content-Length"] = std::to_string(body.length());
            } else {
                // Get body.
                std::string req_body = request.get_body();
                // Body is like "login=123&pass=asd".
                // Parse the req_body.
                std::unordered_map<std::string, std::string> body_map;
                int pos = 0;
                while ((pos = req_body.find('&')) != std::string::npos) {
                    std::string pair = req_body.substr(0, pos);
                    int pos2 = pair.find('=');
                    std::string key = pair.substr(0, pos2);
                    std::string value = pair.substr(pos2 + 1);
                    body_map.insert_or_assign(key, value);
                    req_body.erase(0, pos + 1);
                }
                int pos2 = req_body.find('=');
                int pos3 = req_body.find('.');
                std::string key = req_body.substr(0, pos2);
                std::string value = req_body.substr(pos2 + 1, pos3 - pos2 - 1);
                body_map.insert_or_assign(key, value);

                // Check if the login and pass exist.
                if (
                    body_map.find("login") != body_map.end() &&
                    body_map.find("pass") != body_map.end()
                ) {
                    // Check if the login and pass are correct.
                    if (
                        body_map.at("login") == USERNAME &&
                        body_map.at("pass") == PASSWORD
                    ) {
                        // If the login and pass are correct, return 200.
                        status_code = StatusCodes::OK;
                        // Prepare the 200 response body.
                        body = "<html><body><h1>Login success</h1></body></html>";
                    } else {
                        // If the login and pass are incorrect, return 403.
                        status_code = StatusCodes::FORBIDDEN;
                        // Prepare the 403 response body.
                        body = "<html><body><h1>403 Forbidden (incorrect login or password)</h1></body></html>";
                    }

                    // Prepare the response headers.
                    headers["Content-Type"] = "text/html";
                    headers["Content-Length"] = std::to_string(body.length());

                } else {
                    // If the login and pass do not exist, return 400.
                    status_code = StatusCodes::BAD_REQUEST;

                    // Prepare the 400 response body.
                    body = "<html><body><h1>400 Bad Request</h1></body></html>";

                    // Prepare the 400 response headers.
                    headers["Content-Type"] = "text/html";
                    headers["Content-Length"] = std::to_string(body.length());
                }
            }
        } else {
            // Log the request.
            output_queue_->push(
                "[INFO] Unknown request from " +
                get_client_addr(client_id, lock)
            );

            // Prepare the response.
            status_code = StatusCodes::BAD_REQUEST;
            
            // Prepare the 400 response body.
            body = "<html><body><h1>400 Bad Request</h1></body></html>";

            // Prepare the 400 response headers.
            headers["Content-Type"] = "text/html";
            headers["Content-Length"] = std::to_string(body.length());
        }

        // Log the response.
        output_queue_->push(
            "[INFO] " +
            status_code_to_string(status_code) +
            request.get_url() +
            " " +
            request.get_version() +
            " from " +
            get_client_addr(client_id, lock)
        );

        // Send the response.
        Response response(
            status_code,
            request.get_version(),
            headers,
            body
        );
        sender->send_response(response);
    }

    // Relock the unique_lock
    clientinfo_list_lock.lock();
    output_queue_->push(
        "[INFO] Sent response to " +
        get_client_addr(client_id, clientinfo_list_lock)
    );

    // Remove the client.
    clientinfo_list_->erase(client_id, clientinfo_list_lock);
}

void Server::join_threads() {
    std::unique_lock<std::mutex> client_recv_list_lock(client_recv_list_->get_mutex());
    for (
        auto it = client_recv_list_->begin(client_recv_list_lock);
        it != client_recv_list_->end(client_recv_list_lock);
        it++
    ) {
        it->second->join();
    }
}

void Server::run() {
    while (running_) {
        try {
            // Wait for clients to connect.
            wait_for_client();
            if (!running_) {
                break;
            }
        } catch (std::exception &e) {
            output_queue_->push("[ERR] " + std::string(e.what()));
        }
    }
}

void Server::stop() {
    output_queue_->push("[INFO] Stopping the server...");
    running_ = false;
    // Close the socket.
    shutdown(sockfd_, SHUT_RDWR);
    // Close all the client connections.
    // Get shared_mutex for clientinfo_list_.
    std::unique_lock<std::mutex> clientinfo_list_lock(clientinfo_list_->get_mutex());
    for (
        auto it = clientinfo_list_->begin(clientinfo_list_lock);
        it != clientinfo_list_->end(clientinfo_list_lock);
        it++
    ) {
        // Send a DISCONNECT REQUEST.
        it->second->get_receiver()->close();
    }
}

bool Server::output_message() {
    if (output_queue_->empty()) {
        return false;
    }
    std::cout << std::endl;
    while (!output_queue_->empty()) {
        std::string output = output_queue_->pop();
        std::cout << output << std::endl;
    }
    return true;
}
