#pragma once
#include <iostream>
#include <vector>
#include <winsock2.h>
#include "Ws2tcpip.h"

#include "IBJGameInterconnection.h"
#include "BJLog.h"
#include "BJMessage.h"
#include "threadsafe_queue.h"

class BJSocket : public IBJGameInterconnection
{
private:
	std::stringstream incoming_message;
	void send_outgoing_routine();

protected:
    SOCKET own_socket;
    sockaddr_in own_addr;
    std::vector<WSAEVENT> ahEvents;
    std::vector<SOCKET> sockets;
    bool is_server;
    threadsafe_queue<BJMessage> incoming;
    threadsafe_queue<BJMessage> outgoing;

	// transferring message size
    static const int buffer_size = 128;
	static const char message_delimiter = '#';
    
    BJLog bjlog;

    
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

