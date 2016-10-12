#include "BJMessage.h"




BJMessage::~BJMessage()
{
}

BJMessage::BJMessage(char * data)
{
    throw std::exception("Not implemented");
}

BJMessage::BJMessage(SOCKET socket, char * data, int data_size)
{
    remote_socket = socket;
    message = std::shared_ptr<char>(new char[data_size]);
    memcpy_s(message.get(), data_size * sizeof(char), data, data_size * sizeof(char));
    message_size = data_size;
}

BJMessage::BJMessage(SOCKET socket, std::string str)
{
    remote_socket = socket;
    // TODO : how it works with different encodings
    message_size = str.length();
    message = std::shared_ptr<char>(new char[message_size]);
    memcpy_s(message.get(), message_size * sizeof(char), str.data(), message_size * sizeof(char));
    
}

SOCKET BJMessage::socket()
{
    return remote_socket;
}

int BJMessage::size()
{
    return message_size;
}

std::shared_ptr<char> BJMessage::get_message()
{
    return message;
}

std::string BJMessage::get_message_string()
{
    return std::string((const char*)message.get(), message_size);
}
