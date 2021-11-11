#include "http.h"
#include <sstream>

const std::string http_version{"HTTP/1.1"};

const std::unordered_map<int, std::string> status_map
{
    {200, "OK"},
    {301, "Moved Permanently"},
    {400, "Bad Request"},
    {404, "Not Found"}
};

/**
 * Given string representation of a HTTP message, it identifies whether it is a request or a response.
 * */
Type message_type(const std::string& msg)
{
    return (msg.substr(0, http_version.size()) == http_version) ? Type::Response : Type::Request;
}

/**
 * Common functionality parsing between requests and responses.
 * */
template<Type T>
static HTTP<T> parse_header_body(std::stringstream& line_stream, HTTP_Builder<T>& builder)
{
    std::string line, token;
    while(std::getline(line_stream, line))
    {
        // Pop /r
        line.pop_back();
        // Headers ended, next line is from message body.
        if(line.empty())
        {
            break;
        }
        std::stringstream stream{line};
        // Read header value
        stream >> token;
        // Pop :
        token.pop_back();
        std::string header_name = std::move(token);
        stream >> token;
        builder.addHeader(header_name, token);
    }

    builder.addBody(std::string (std::istreambuf_iterator<char>(line_stream), {}));
    return builder.build();
}

/**
 * Converts string representation of a HTTP request into a HTTP request object.
 * */
HTTP<Type::Request> read_request(const std::string& msg)
{
    std::stringstream line_stream(msg);
    HTTP_Builder<Type::Request> builder;
    std::string line, token;
    std::getline(line_stream, line);
    // First line in HTTP request.
    std::stringstream stream(line);
    // Add Command.
    stream >> token;
    builder.setCommand(token);
    // Add URL for the request.
    stream >> token;
    builder.setURL(token);
    return parse_header_body(line_stream, builder);
}

/**
 * Converts string representation of a HTTP response into a HTTP response object.
 * */
HTTP<Type::Response> read_response(const std::string& msg)
{
    std::stringstream line_stream(msg);
    HTTP_Builder<Type::Response> builder;
    std::string line, token;
    std::getline(line_stream, line);
    // First line in HTTP request.
    std::stringstream stream(line);
    // Skip version
    stream >> token;
    // Get status
    stream >> token;
    builder.setStatus(std::stoi(token));
    return parse_header_body(line_stream, builder);
}
