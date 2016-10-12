#pragma once
#include <winsock2.h>
#include <memory>
#include <string>


class BJMessage
{
private:
    SOCKET remote_socket;
    int message_size;
    std::shared_ptr<char> message;
public:
    BJMessage() = delete;
    ~BJMessage();

    BJMessage(char*);
    BJMessage(SOCKET, char*, int);
    BJMessage(SOCKET, std::string);

    SOCKET socket();
    int size();
    std::shared_ptr<char> get_message();
    std::string get_message_string();
};

