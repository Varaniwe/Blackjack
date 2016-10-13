#pragma once
#include <mutex>

#include "BJSocket.h"

class BJServer : public BJSocket
{
public:
    BJServer();
    ~BJServer();

    bool BJServerInit(int port);
    
    void AcceptConnection(int client_number) override;
    //void ReceiveMessage(int socket_number) override;
    void CloseConnection(int client_number) override;

    void BJSendMessage(char* data, int data_size, SOCKET sckt) override;
    void BJSendMessage(BJMessage) override;
    void BJSendMessage(std::string) override;
};

