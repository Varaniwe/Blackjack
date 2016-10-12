#include "BJServer.h"



BJServer::BJServer()
{
    is_server = true;
}


BJServer::~BJServer()
{
    
}

bool BJServer::BJServerInit(int port)
{
    if (!BJSocketInit("0.0.0.0", port))
    {
        bjlog.error("Cannot init socket");
        return false;
    }

    int iResult = bind(own_socket, (SOCKADDR *)& own_addr, sizeof(own_addr));
    if (iResult != 0) {
        bjlog.error("bind failed with error: ", WSAGetLastError());
        return false;
    }

    return true;
}


void BJServer::AcceptConnection(int client_number)
{
    sockaddr_in temp_sockaddr;
    memset(&temp_sockaddr, 0, sizeof(temp_sockaddr));

    int temp_sockaddr_len = sizeof(temp_sockaddr);
    sockets.push_back(accept(own_socket, (SOCKADDR *)&temp_sockaddr, &temp_sockaddr_len));
    ahEvents.push_back(WSACreateEvent());
    WSAEventSelect(sockets.back(), ahEvents.back(), (FD_READ | FD_WRITE | FD_CLOSE));
        
    char client_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(temp_sockaddr.sin_addr), client_ip_str, INET_ADDRSTRLEN);

    bjlog.write("Get connection from client", client_ip_str, "port=", (int)ntohs(temp_sockaddr.sin_port));
    
}


void BJServer::CloseConnection(int client_number)
{
    bjlog.write(sockets[client_number], "Client has been disconnected");

    BJMessage msg(sockets[client_number], "player_disconnect:");
    incoming.push(msg);

    sockets.erase(sockets.begin() + client_number);
    ahEvents.erase(ahEvents.begin() + client_number);
}

void BJServer::BJSendMessage(char * data, int data_size, SOCKET sckt)
{
    BJMessage bjmessage(sckt, data, data_size);
    outgoing.push(bjmessage);    
}

void BJServer::BJSendMessage(BJMessage bjmessage)
{
    //bjlog.write(bjmessage.get_message_string());

    outgoing.push(bjmessage);    
}

void BJServer::BJSendMessage(std::string msg)
{
    for (int i = 1; i < sockets.size(); ++i)
    {
        BJMessage bjmessage(sockets[i], msg);
        outgoing.push(bjmessage);
    }
}