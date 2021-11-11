#include <iostream>
#include <chrono>
#include "../debugger.h"
#include "../http.h"
#include "../networking.h"

using namespace std;

constexpr int MAX_SIZE = 15000;

int main()
{
    WSADATA wsaData;

    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        Err("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    vector<unique_ptr<Socket>> sockets;
    for(int i = 0; i < MAX_SIZE; i++){
        sockets.emplace_back(connectToServer("127.0.0.1", "80"));
    }

    double avg_time = 0;
    for(int i = 0; i < MAX_SIZE; i++){
        auto t1 = std::chrono::high_resolution_clock::now();
        HTTP_Builder<Type::Request> builder;
        HTTP<Type::Request> req = builder.setCommand("GET").setURL("/text.txt").build();
        sockets[i]->sendHTTP(req.to_string());
        auto unused = sockets[i]->receiveHTTP();
        auto t2 = std::chrono::high_resolution_clock::now();
        avg_time += 1.0 * std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / MAX_SIZE;
    }

    std::cout << "Using " << MAX_SIZE << " concurrent requests, it takes on avg for a single request " << avg_time << " ms.";
    cout.flush();
    WSACleanup();
    return 0;
}
