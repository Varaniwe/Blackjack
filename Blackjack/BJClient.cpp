#include "BJClient.h"



BJClient::BJClient()
{
    is_server = false;
}


BJClient::~BJClient()
{
}

bool BJClient::BJConnect(LPCSTR ip, int p_port)
{    
    if (!BJSocketInit(ip, p_port))
    {
        bjlog.error("Cannot init socket");
        return false;
    }

    SOCKET server_socket = connect(own_socket, (SOCKADDR *)&own_addr, sizeof(own_addr));
    if (server_socket == INVALID_SOCKET )
    {
        bjlog.error("Couldn't connect", "bind failed with error:", WSAGetLastError());
        return false;
    }
    bjlog.write("Connect...");
    return true;

}

void BJClient::AcceptConnection(int client_number)
{
    bjlog.error("AcceptConnection");
    throw std::exception("AcceptConnection for BJClient is not implemented");
}


void BJClient::CloseConnection(int socket_number)
{
    bjlog.error("CloseConnection");
    sockets.erase(sockets.begin() + socket_number);
    ahEvents.erase(ahEvents.begin() + socket_number);
}


void BJClient::BJSendMessage(char * data, int data_size, SOCKET sckt)
{
    BJMessage bjmessage(sckt, data, data_size);
    outgoing.push(bjmessage);
}

void BJClient::BJSendMessage(BJMessage bjmessage)
{
    outgoing.push(bjmessage);
}

void BJClient::BJSendMessage(std::string msg)
{
    BJMessage bjmessage(sockets[0], msg);
    outgoing.push(bjmessage);
}