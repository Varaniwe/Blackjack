#pragma once
#include "BJSocket.h"
class BJClient : public BJSocket
{
public:
    BJClient();
    ~BJClient();

    bool BJConnect(LPCSTR ip, int p_port);


    void AcceptConnection(int client_number) override;
    //void ReceiveMessage(int client_number) override;
    void CloseConnection(int client_number) override;

    void BJSendMessage(char* data, int data_size, SOCKET sckt) override;
    void BJSendMessage(BJMessage) override;
    void BJSendMessage(std::string) override;
};

