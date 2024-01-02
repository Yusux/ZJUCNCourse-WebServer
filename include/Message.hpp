#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "def.hpp"
#include <vector>
#include <unordered_map>
#include <string>

enum class StatusCodes {
    UNKNOWN=-1,
    OK=200,
    BAD_REQUEST=400,
    FORBIDDEN=403,
    NOT_FOUND=404,
    INTERNAL_SERVER_ERROR=500
};

enum class MethodTypes {
    UNKNOWN=-1,
    GET=0,
    POST=1
};

std::string status_code_to_string(const StatusCodes& status_code);
std::string method_type_to_string(const MethodTypes& method_type);

class Message {
private:
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;

public:
    Message() = delete;

    /*
     * @brief Construct a new Message object
     * @param version HTTP version
     * @param body HTTP body
     * @param headers HTTP headers
     */
    Message(
        const std::string& version,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers
    );

    /*
     * @brief Construct a new Message object
     * @param other Message object to copy
     */
    Message(const Message& other);

    /*
     * @brief Destroy the Message object
     */
    ~Message();

    /*
     * @brief Get the version object
     * @return std::string HTTP version
     */
    std::string get_version() const;

    /*
     * @brief Get the body object
     * @return std::string HTTP body
     */
    std::string get_body() const;

    /*
     * @brief Get the headers object
     * @return std::unordered_map<std::string, std::string> HTTP headers
     */
    std::unordered_map<std::string, std::string> get_headers() const;

    Message& operator=(const Message& other);
};

class Request : public Message {
private:
    MethodTypes method_type_;
    std::string url_;
public:
    Request();

    /*
     * @brief Construct a new Request object
     * @param method_type HTTP method type
     * @param url HTTP url
     * @param version HTTP version
     * @param body HTTP body
     * @param headers HTTP headers
     */
    Request(
        const MethodTypes& method_type,
        const std::string& url,
        const std::string& version,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers
    );

    /*
     * @brief Construct a new Request object
     * @param other Request object to copy
     */
    Request(const Request& other);

    /*
     * @brief Destroy the Request object
     */
    ~Request();

    /*
     * @brief Get the method type object
     * @return MethodTypes HTTP method type
     */
    MethodTypes get_method_type() const;

    /*
     * @brief Get the url object
     * @return std::string HTTP url
     */
    std::string get_url() const;

    /*
     * @brief Convert the Request object to a string
     * @return std::string The string representation of the Request object
     */
    std::string to_string() const;

    Request& operator=(const Request& other);
};

class Response : public Message {
private:
    StatusCodes status_code_;
public:
    Response();

    /*
     * @brief Construct a new Response object
     * @param status_code HTTP status code
     * @param version HTTP version
     * @param headers HTTP headers
     * @param body HTTP body
     */
    Response(
        const StatusCodes& status_code,
        const std::string& version,
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& body
    );

    /*
     * @brief Construct a new Response object
     * @param other Response object to copy
     */
    Response(const Response& other);

    /*
     * @brief Destroy the Response object
     */
    ~Response();

    /*
     * @brief Get the status code object
     * @return StatusCodes HTTP status code
     */
    StatusCodes get_status_code() const;

    /*
     * @brief Serialize the Response object
     * @param buffer The buffer to serialize the Response object to
     * @return ssize_t The number of bytes serialized
     */
    ssize_t serialize(std::vector<uint8_t>& buffer) const;

    /*
     * @brief Convert the Response object to a string
     * @return std::string The string representation of the Response object
     */
    std::string to_string() const;

    Response& operator=(const Response& other);
};

#endif
