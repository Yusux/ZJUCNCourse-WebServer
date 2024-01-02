#include "Message.hpp"
#include <stdexcept>

std::string status_code_to_string(const StatusCodes& status_code) {
    switch (status_code) {
        case StatusCodes::OK:
            return "200 OK";
        case StatusCodes::BAD_REQUEST:
            return "400 Bad Request";
        case StatusCodes::FORBIDDEN:
            return "403 Forbidden";
        case StatusCodes::NOT_FOUND:
            return "404 Not Found";
        case StatusCodes::INTERNAL_SERVER_ERROR:
            return "500 Internal Server Error";
        default:
            throw std::invalid_argument("Invalid status code");
    }
}

std::string method_type_to_string(const MethodTypes& method_type) {
    switch (method_type) {
        case MethodTypes::GET:
            return "GET";
        case MethodTypes::POST:
            return "POST";
        default:
            throw std::invalid_argument("Invalid method type");
    }
}

Message::Message(
    const std::string& version,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers
) : version_(version), body_(body), headers_(headers) {}

Message::Message(const Message& other) {
    this->version_ = other.version_;
    this->body_ = other.body_;
    this->headers_ = other.headers_;
}

Message::~Message() {}

std::string Message::get_version() const {
    return this->version_;
}

std::string Message::get_body() const {
    return this->body_;
}

std::unordered_map<std::string, std::string> Message::get_headers() const {
    return this->headers_;
}

Message& Message::operator=(const Message& other) {
    this->version_ = other.version_;
    this->body_ = other.body_;
    this->headers_ = other.headers_;
    return *this;
}

Request::Request() : Message("", "", {}), method_type_(MethodTypes::UNKNOWN), url_("") {}

Request::Request(
    const MethodTypes& method_type,
    const std::string& url,
    const std::string& version,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers
) : Message(version, body, headers), method_type_(method_type), url_(url) {}

Request::Request(const Request& other) : Message(other) {
    this->method_type_ = other.method_type_;
    this->url_ = other.url_;
}

Request::~Request() {}

MethodTypes Request::get_method_type() const {
    return this->method_type_;
}

std::string Request::get_url() const {
    return this->url_;
}

std::string Request::to_string() const {
    std::string message = method_type_to_string(get_method_type()) + " " + get_url() + " " + get_version() + "\r\n";
    std::unordered_map<std::string, std::string> headers = get_headers();
    for (auto& header : headers) {
        message += header.first + ": " + header.second + "\r\n";
    }
    message += "\r\n";
    message += get_body();
    return message;
}

Request& Request::operator=(const Request& other) {
    this->method_type_ = other.method_type_;
    this->url_ = other.url_;
    Message::operator=(other);
    return *this;
}

Response::Response() : Message("", "", {}), status_code_(StatusCodes::UNKNOWN) {}

Response::Response(
    const StatusCodes& status_code,
    const std::string& version,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& body
) : Message(version, body, headers), status_code_(status_code) {}

Response::Response(const Response& other) : Message(other) {
    this->status_code_ = other.status_code_;
}

Response::~Response() {}

StatusCodes Response::get_status_code() const {
    return this->status_code_;
}

ssize_t Response::serialize(std::vector<uint8_t>& buffer) const {
    std::string message = get_version() + " " + status_code_to_string(get_status_code()) + "\r\n";
    for (auto& header : get_headers()) {
        message += header.first + ": " + header.second + "\r\n";
    }
    message += "\r\n";
    message += get_body();
    buffer.resize(message.size());
    std::copy(message.begin(), message.end(), buffer.begin());
    return message.size();
}

std::string Response::to_string() const {
    std::string message = get_version() + " " + status_code_to_string(get_status_code()) + "\r\n";
    for (auto& header : get_headers()) {
        message += header.first + ": " + header.second + "\r\n";
    }
    message += "\r\n";
    message += get_body();
    return message;
}

Response& Response::operator=(const Response& other) {
    this->status_code_ = other.status_code_;
    Message::operator=(other);
    return *this;
}
