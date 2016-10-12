#pragma once
#include <winsock2.h>
#include <iostream>
#include "Ws2tcpip.h"
#include <mutex>
#include <vector>

#include "BJLog.h"
#include "threadsafe_queue.h"
#include "BJMessage.h"
#include "IBJGameInterconnection.h"

class BJSocket : public IBJGameInterconnection
{
protected:
    SOCKET own_socket;
    sockaddr_in own_addr;
    std::vector<WSAEVENT> ahEvents;
    std::vector<SOCKET> sockets; 
    bool is_server;
    threadsafe_queue<BJMessage> incoming;
    threadsafe_queue<BJMessage> outgoing;

    static const int buffer_size = 128;
    
    BJLog bjlog;

    std::mutex sockets_mutex;
    
public:
    BJSocket();
    ~BJSocket();
    bool BJSocketInit(LPCSTR ip, int port);
    void BJWaitForEvents();

    virtual void BJSendMessage(char* data, int data_size, SOCKET sckt) = 0;
    virtual void BJSendMessage(BJMessage) = 0;
    virtual void BJSendMessage(std::string) = 0;

    BJMessage Receive() override;
    
    virtual void AcceptConnection(int client_number) = 0;
    void ReceiveMessage(int client_number);
    virtual void CloseConnection(int client_number) = 0;
};

