#include <bits/stdc++.h>
#include "Http.h"
#include "networking.h"
#include "debugger.h"

using namespace std;

const std::map<std::string, std::string> extension_map{
    {
        "txt", "text/plain"
    },
    {
        "html", "text/html"
    },
    {
            "css", "text/css"
    },
    {
            "js", "text/js"
    },
    {
        "jpeg", "img/jpeg"
    },
    {
        "png", "img/png"
    }
};

int main()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        Err("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    Server::Handler handler = [&](const HTTP<Type::Request>& req)
    {
        HTTP_Builder<Type::Response> builder;
        auto url = req.get_url();
        url = url.substr(1);
        if(req.get_command() == "GET")
        {
            std::ifstream file(url, std::ios::in | std::ios::binary);
            if(!file.is_open())
            {
                Err("%s : failed to open file", url.c_str());
                return builder.setStatus(404).build();
            }
            file.seekg(0, std::ios::end);
            std::size_t length = file.tellg();
            file.seekg(0, std::ios::beg);
            if (file.fail())
            {
                Err("%s : failed to get size of file", url.c_str());
                return builder.setStatus(404).build();
            }
            std::string data(length, '\0');
            if(!file.read(&data[0], length))
            {
                Err("%s : failed to read file", url.c_str());
                return builder.setStatus(404).build();
            }
            if (file){
              Debug("%s : all characters read successfully", url.c_str());
            }else{\
              Err("%s : only %d could be read", url.c_str(), file.gcount());
              return builder.setStatus(404).build();
            }
            int position = url.find_last_of(".");

            string extension = url.substr(position+1);
            return builder.setStatus(200).addHeader("Content-Type", extension_map.at(extension)).addHeader("Connection", "Keep-Alive")
            .addHeader("Content-Length", std::to_string(length)).addBody(data).build();
        }else if(req.get_command() == "POST"){
            // Check and create directories
            int position = url.find_last_of("/");
            if(position != -1){
                string directories = url.substr(0, position);
                bool success = std::filesystem::create_directories(directories);
                if(!success){
                    Err("%s : failed to create directories");
                    return builder.setStatus(404).build();
                }
            }
            std::ofstream file(url, std::ios::out | std::ios::trunc | std::ios::binary);
            if(!file.is_open())
            {
                Err("%s : failed to open file", url.c_str());
                return builder.setStatus(404).build();
            }
            Debug("%s : writing %d chars", url.c_str(), req.get_body().size());
            file << req.get_body();
            if(file){
                Debug("%s : written successfully", url.c_str());
            }else{
                Err("%s : error while writing data", url.c_str());
                return builder.setStatus(404).build();
            }
            return builder.setStatus(200).build();
        }
        return builder.setStatus(400).addHeader("Connection", "Keep-Alive").build();
    };
    try
    {
        Server serv("80", handler);
        serv.ListenAndServe();
    }
    catch(std::exception& e)
    {
        cerr << e.what();
    }

    WSACleanup();
    return 0;
}
