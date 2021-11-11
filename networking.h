#ifndef NETWORKING_H_INCLUDED
#define NETWORKING_H_INCLUDED

#define _WINNT_WIN32 0x0601

#include "Http.h"
#include <ws2tcpip.h>
#include <winsock2.h>
#include <functional>
#include <atomic>
#include <string>
#include <array>
#include <memory>

// Relatively high value
constexpr int default_timeout = 1000;

/**
 * A Wrapper class for the raw socket supplied by winsock to extend functionality.
 */
class Socket
{
public:
    /**
     * Creates a wrapper around SOCKET type.
     */

    Socket(SOCKET socket);

    /**
     * Destroys the underlying SOCKET (RAII).
     */
    ~Socket();

    /**
     * Optionally returns a HTTP message as a string using a blocking receive
     * with a timeout.
     */
    std::optional<std::string> receiveHTTP(int timeout_seconds = default_timeout);

    /**
     * A blocking send for HTTP messages.
     */
    bool sendHTTP(const std::string& req);

    /**
     * Shutdown sending for this socket.
     */
    bool shutdownSender();

    /**
     * Returns the underlying socket.
     */
    SOCKET getRawSocket();
private:
    SOCKET socket_;
    static constexpr int buffer_size = 1 << 16;
    // Buffer used to receive data.
    char buff[buffer_size];
    // Has previous buffer of the previous request.
    std::string prev_buff;
};

/**
 * Connects to the given addr and port and returns the socket associated with the connection.
 */
std::unique_ptr<Socket> connectToServer(const char* addr, const char* port);

/**
 * Server class to listen to a specific port that handles multiple connections and HTTP requests concurrently
 * with a customized handler function.
 */
class Server
{
public:
    using Handler = std::function<HTTP<Type::Response>(const HTTP<Type::Request>&)>;

    /**
     * Creates a server with the specified port and the customized handler.
     */
    Server(const char *port, Handler handler);

    /**
     * A blocking function call that administers the server to start listening and serving requests.
     */
    [[noreturn]] [[noreturn]] void ListenAndServe();
private:
    /**
     *  A blocking function call that waits for connections and then returns
     *  the socket associated with that connection.
     */
    std::unique_ptr<Socket> acceptConnection();

    /**
     *  Given a socket it instructs the current thread to handle all requests for this connection until
     *  the connection is closed or a timeout happens.
     */
    void serveConnection(std::unique_ptr<Socket> socket);
    std::unique_ptr<Socket> ListenSocket_;
    // Customized handler initialized with the server to serve requests.
    Handler handler;
    // Number of open connections.
    std::atomic<int> n_connections{0};
};


#endif // NETWORKING_H_INCLUDED
