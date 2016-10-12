#pragma once
#include "BJMessage.h"

class IBJGameInterconnection
{
public:
    IBJGameInterconnection() = default;
    ~IBJGameInterconnection() = default;

    virtual void BJSendMessage(char * data, int data_size, SOCKET sckt) = 0;
    virtual void BJSendMessage(BJMessage) = 0;
    virtual void BJSendMessage(std::string) = 0;

    virtual BJMessage Receive() = 0;
};