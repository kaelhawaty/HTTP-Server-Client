#include <bits/stdc++.h>
#include "../Http.h"
#include "../networking.h"
#include "../debugger.h"

using namespace std;

const std::map<std::string, std::string> extension_map
        {
                {
                        "txt",  "text/plain"
                },
                {
                        "html", "text/html"
                },
                {
                        "jpeg", "img/jpeg"
                },
                {
                        "png",  "img/png"
                }
        };


int main(int argc, char *argv[]) {
    if (argc != 2) {
        Err("Only 1 argument is needed which is the filename");
        return 0;
    }
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        Err("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    char *filename = argv[1];
    std::ifstream stream{filename};
    while (!stream.eof()) {
        std::string command, path, hostname, port;
        stream >> command >> path >> hostname >> port;
        auto socket_ptr = connectToServer(hostname.c_str(), port.c_str());
        if (command == "client_get") {
            HTTP_Builder<Type::Request> builder;
            HTTP<Type::Request> req = builder.setCommand("GET").setURL(path).addHeader("Connection", "Keep-Alive").build();
            bool success = socket_ptr->sendHTTP(req.to_string());
            if (!success) {
                Err("%s : failed to send HTTP Request", path.c_str());
                continue;
            }
            auto resp_str_opt = socket_ptr->receiveHTTP();
            if (!resp_str_opt) {
                continue;
            }
            HTTP<Type::Response> resp = read_response(*resp_str_opt);
            path = path.substr(1);
            int position = path.find_last_of("/");
            if (position != -1) {
                string directories = path.substr(0, position);
                bool success = std::filesystem::create_directories(directories);
                if (!success) {
                    Err("%s : failed to create directories", directories.c_str());
                    continue;
                }
            }
            std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!file.is_open()) {
                Err("%s : failed to open file", path.c_str());
                continue;
            }
            Debug("%s : writing %d chars", path.c_str(), resp.get_body().size());
            Debug("\n-------------------------\n %s \n-------------------------\n", req.to_string(false).c_str());
            file << resp.get_body();
            if (file) {
                Debug("%s : written successfully", path.c_str());
            } else {
                Err("%s : error while writing data", path.c_str());
            }
        } else {
            std::ifstream file(path.substr(1), std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                Err("%s : failed to open file", path.c_str());
                continue;
            }
            file.seekg(0, std::ios::end);
            std::size_t length = file.tellg();
            file.seekg(0, std::ios::beg);
            if (file.fail()) {
                Err("%s : failed to get size of file", path.c_str());
                continue;
            }
            std::string data(length, '\0');
            if (!file.read(&data[0], length)) {
                Err("%s : failed to read file", path.c_str());
                continue;
            }
            if (file) {
                Debug("%s : all characters read successfully", path.c_str());
            } else {
                Err("%s : only %d could be read", path.c_str(), file.gcount());
                continue;
            }
            int position = path.find_last_of(".");
            string extension = path.substr(position + 1);
            HTTP_Builder<Type::Request> builder;
            auto req = builder.setCommand("POST").setURL(path).addBody(move(data))
                    .addHeader("Content-Type", extension_map.at(extension)).addHeader("Connection", "Keep-Alive")
                    .addHeader("Content-Length", std::to_string(length)).build();
            bool success = socket_ptr->sendHTTP(req.to_string());
            if (!success) {
                Err("%s : failed to send HTTP Request", path.c_str());
                continue;
            }
            auto resp_str_opt = socket_ptr->receiveHTTP();
            if (!resp_str_opt) {
                Err("%s : failed to receive HTTP response", path.c_str());
                continue;
            }
            Debug("\n-------------------------\n %s \n-------------------------\n", resp_str_opt->c_str());
        }
    }
    WSACleanup();

    return 0;
}
