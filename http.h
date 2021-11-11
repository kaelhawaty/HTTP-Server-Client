#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include <type_traits>
#include <unordered_map>
#include <map>
#include <string>

enum class Type{Request, Response};

extern const std::string http_version;
extern const std::unordered_map<int, std::string> status_map;

template<Type T>
class HTTP_Builder;

/**
 * A base class to share common functionality between HTTP requests and responses.
 * */
template<Type T>
class HTTP_Base{
public:
    const std::string& get_version() const{
        return http_version;
    }
    const std::string& get_header(const std::string& header) const{
        return header_map.at(header);
    }
    bool has_header(const std::string& header) const{
        return header_map.find(header) != header_map.end();
    }
    const std::string& get_body() const{
        return body;
    }

    std::string to_string(bool include_body = true) const {
        std::string text;
        for(const auto& ent: header_map){
            text += ent.first + ": " + ent.second + "\r\n";
        }
        text += "\r\n";
        if(include_body){
            text += body;
        }
        return text;
    }
private:
    friend class HTTP_Builder<T>;
    std::map<std::string, std::string> header_map;
    std::string body;
};

template<Type T>
class HTTP : public HTTP_Base<T> {};

/**
 * Represents of HTTP Request.
 * */
template<>
class HTTP<Type::Request> : public HTTP_Base<Type::Request>{
public:
    const std::string& get_command() const{
        return command;
    }
    const std::string& get_url() const{
        return url;
    }
    std::string to_string(bool include_body = true) const {
        std::string text;
        text += command + " " + url + " " + this->HTTP_Base<Type::Request>::get_version() + "\r\n";
        text += this->HTTP_Base<Type::Request>::to_string(include_body);
        return text;
    }
private:
    friend class HTTP_Builder<Type::Request>;
    std::string command, url;
};

/**
 * Represents of HTTP response.
 * */
template<>
class HTTP<Type::Response> : public HTTP_Base<Type::Response>{
public:
    const std::string& get_status() const{
        return status;
    }

    std::string to_string(bool include_body = true) const {
        std::string text;
        text += this->HTTP_Base<Type::Response>::get_version() + " " + status + "\r\n";
        text += this->HTTP_Base<Type::Response>::to_string(include_body);
        return text;
    }
private:
    friend class HTTP_Builder<Type::Response>;
    std::string status;
};


/**
 * A Builder for HTTP Requests and responses
 * */
template<Type T>
class HTTP_Builder{
public:
    template<Type T_ = T, std::enable_if_t<T_ == Type::Response && T_ == T>* = nullptr>
    HTTP_Builder<T>& setStatus(int status){
        http.status = std::to_string(status) + " " + status_map.at(status);
        return *this;
    }

    template<Type T_ = T, std::enable_if_t<T_ == Type::Request && T_ == T>* = nullptr>
    HTTP_Builder<T>& setCommand(std::string command){
        http.command = std::move(command);
        return *this;
    }

    template<Type T_ = T, std::enable_if_t<T_ == Type::Request && T_ == T>* = nullptr>
    HTTP_Builder<T>& setURL(std::string url){
        http.url = std::move(url);
        return *this;
    }

    HTTP_Builder<T>& addHeader(std::string name, std::string value){
        http.header_map.emplace(std::move(name), std::move(value));
        return *this;
    }

    HTTP_Builder<T>& addBody(std::string body){
        http.body = std::move(body);
        return *this;
    }

    HTTP<T> build(){
        return std::move(http);
    }
private:
    HTTP<T> http;
};

/**
 * Given string representation of a HTTP message, it identifies whether it is a request or a response.
 * */
Type message_type(const std::string& msg);

/**
 * Converts string representation of a HTTP request into a HTTP request object.
 * */
HTTP<Type::Request> read_request(const std::string& msg);

/**
 * Converts string representation of a HTTP response into a HTTP response object.
 * */
HTTP<Type::Response> read_response(const std::string& msg);




#endif // HTTP_H_INCLUDED
