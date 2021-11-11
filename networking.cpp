#include "networking.h"
#include "debugger.h"
#include <stdexcept>
#include <optional>
#include <iostream>
#include <thread>

constexpr int total_timeout_seconds = 50;

/**
* Creates a server with the specified port and the customized handler.
*/
Server::Server(const char *port, Handler handler) : handler(handler) {
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0) {
        std::string err_msg = "getaddrinfo failed: " + std::to_string(iResult) + "\n";
        throw std::runtime_error(err_msg);
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        std::string err_msg = "Error at socket(): " + std::to_string(WSAGetLastError()) + "\n";
        freeaddrinfo(result);
        throw std::runtime_error(err_msg);
    }

    ListenSocket_ = std::make_unique<Socket>(ListenSocket);


    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::string err_msg = "bind failed with error: " + std::to_string(WSAGetLastError()) + "\n";
        freeaddrinfo(result);
        throw std::runtime_error(err_msg);
    }

    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::string err_msg = "Listen failed with error: " + std::to_string(WSAGetLastError()) + "\n";
        throw std::runtime_error(err_msg);
    }
}

/**
*  A blocking function call that waits for connections and then returns
*  the socket associated with that connection.
*/
std::unique_ptr<Socket> Server::acceptConnection() {
    SOCKET ClientSocket = INVALID_SOCKET;
    // Accept a client socket
    ClientSocket = accept(ListenSocket_->getRawSocket(), NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        Err("accept failed: %d", WSAGetLastError());
        return nullptr;
    }
    return std::make_unique<Socket>(ClientSocket);
}

/**
* A blocking function call that administers the server to start listening and serving requests.
*/
[[noreturn]] void Server::ListenAndServe() {
    while (true) {
        auto socket_ptr = acceptConnection();
        std::thread thread(&Server::serveConnection, this, std::move(socket_ptr));
        thread.detach();
    }
}

/**
*  Given a socket it instructs the current thread to handle all requests for this connection until
*  the connection is closed or a timeout happens.
*/
void Server::serveConnection(std::unique_ptr<Socket> socket) {
    n_connections++;
    while (true) {
        int timeout = total_timeout_seconds / n_connections;
        auto req_str_opt = socket->receiveHTTP(timeout);
        if (!req_str_opt) {
            break;
        }
        HTTP<Type::Request> req = read_request(*req_str_opt);
        Debug("\n-------------------------\n %s \n-------------------------\n", req.to_string(false).c_str());
        auto resp = handler(req);
        bool success = socket->sendHTTP(resp.to_string());
        if (!success) {
            Err("failed to send HTTP response");
            break;
        }
    }
    n_connections--;
}

/**
 * Connects to the given addr and port and returns the socket associated with the connection.
 */
std::unique_ptr<Socket> connectToServer(const char *addr, const char *port) {
    // Get address information.
    struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult = getaddrinfo(addr, port, &hints, &result);
    if (iResult != 0) {
        Err("getaddrinfo failed: %d", iResult);
        return nullptr;
    }

    SOCKET socket_ = INVALID_SOCKET;

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_ == INVALID_SOCKET) {
            Err("socket failed with error: %d", WSAGetLastError());
            freeaddrinfo(result);
            return nullptr;
        }

        // Connect to server.
        iResult = connect(socket_, ptr->ai_addr, (int) ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (socket_ == INVALID_SOCKET) {
        Err("Unable to connect to the server %s with port %s", addr, port);
        return nullptr;
    }
    return std::make_unique<Socket>(socket_);
}

/**
 * Creates a wrapper around SOCKET type.
 */
Socket::Socket(SOCKET socket) {
    this->socket_ = socket;
}

/**
 * Destroys the underlying SOCKET (RAII).
 */
Socket::~Socket() {
    closesocket(socket_);
}

/**
 * A blocking send for HTTP messages.
 */
bool Socket::sendHTTP(const std::string &req) {
    int iResult = send(socket_, req.c_str(), (int) req.size(), 0);
    if (iResult == SOCKET_ERROR) {
        Err("send failed: %d", WSAGetLastError());
        return false;
    }
    Debug("Bytes Sent: %d", iResult);
    return true;
}

/**
 * Optionally returns a HTTP message as a string using a blocking receive
 * with a timeout.
 */
std::optional<std::string> Socket::receiveHTTP(int timeout_seconds) {
    std::string received;
    DWORD timeout = timeout_seconds * 1000;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
    int iResult, content_length = 0, total_size = 0;
    bool header_ended = false;
    do {
        if(!prev_buff.empty()){
            received += prev_buff;
            prev_buff.clear();
        }else {
            iResult = recv(socket_, buff, buffer_size, 0);
            if (iResult > 0) {
                Debug("Bytes received: %d", iResult);
                for (int i = 0; i < iResult; i++) {
                    received += buff[i];
                }
            } else if (iResult == -1 || iResult == 0) {
                Debug("Connection closed");
                return std::nullopt;
            } else {
                Err("recv failed: ", WSAGetLastError());
                return std::nullopt;
            }
        }
        if (!header_ended) {
            int idx = received.find("\r\n\r\n", received.size() - iResult);
            // HTTP Header ended.
            if (idx != std::string::npos) {
                Type t = message_type(received);
                if (t == Type::Request) {
                    auto req = read_request(received);
                    if (req.has_header("Content-Length")) {
                        content_length = std::stoi(req.get_header("Content-Length"));
                    }
                } else {
                    auto req = read_response(received);
                    if (req.has_header("Content-Length")) {
                        content_length = std::stoi(req.get_header("Content-Length"));
                    }
                }
                total_size = content_length + idx + 4;
                // Subtract already received content
                content_length -= received.size() - (idx + 4);
                if (content_length <= 0) {
                    break;
                }
                header_ended = true;
            }
        } else {
            content_length -= std::min(content_length, iResult);
            if (content_length <= 0) {
                break;
            }
        }
    } while (iResult > 0);
    prev_buff = received.substr(total_size);
    received.resize(total_size);
    return received;
}

/**
 * Returns the underlying socket.
 */
SOCKET Socket::getRawSocket() {
    return socket_;
}

/**
 * Shutdown sending for this socket.
 */
bool Socket::shutdownSender() {
    // shutdown the connection for sending since no more data will be sent
    // the client can still use the ConnectSocket for receiving data
    int iResult = shutdown(socket_, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        Err("shutdown failed: ", WSAGetLastError());
        closesocket(socket_);
        return false;
    }
    return true;
}

