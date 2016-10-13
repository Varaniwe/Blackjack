#include "BJSocket.h"


BJSocket::BJSocket() : bjlog(std::cout)
{
    own_socket = INVALID_SOCKET;
    bjlog.set_min_log_level(LogLevel::debug);
}


BJSocket::~BJSocket()
{
    for(auto s : sockets)
    {
        if (s != INVALID_SOCKET && s != SOCKET_ERROR)
        {
            closesocket(s);
        }
    }
}

bool BJSocket::BJSocketInit(LPCSTR ip, int port)
{
    int iResult;
    WSADATA wsaData;

    memset(&own_socket, 0, sizeof(own_socket));

    own_addr.sin_family = AF_INET; // address family Internet
    own_addr.sin_port = htons(port); //Port to connect on
    if (is_server)
    {
        own_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        inet_pton(AF_INET, ip, &(own_addr.sin_addr));
    }

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        bjlog.error("WSAStartup failed with error:", iResult);
        return false;
    }

    //---------------------------------------------
    // Create a socket for sending data
    own_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (own_socket == INVALID_SOCKET) {
        bjlog.error("socket failed with error:", WSAGetLastError());
        WSACleanup();
        return false;
    }
    
    return true;
}

void BJSocket::BJWaitForEvents()
{
    WSANETWORKEVENTS NetworkEvents;
    ahEvents.clear();
    ahEvents.push_back(WSACreateEvent());
    sockets.clear();
    sockets.push_back(own_socket);

    int iResult = 0;
    
    if (is_server)
    {
        iResult = WSAEventSelect(sockets[0], ahEvents[0], (FD_ACCEPT | FD_CLOSE));
        if (iResult == SOCKET_ERROR)
        {
            bjlog.error("WSAEventSelect() error:", WSAGetLastError());
            exit(1);
        }

        iResult = listen(sockets[0], SOMAXCONN);
        if (iResult != 0) {
            bjlog.error("listen failed with error:", WSAGetLastError());
            exit(1);
        }
    }    
    else
    {
        iResult = WSAEventSelect(sockets[0], ahEvents[0], (FD_READ | FD_WRITE | FD_CLOSE));
        if (iResult == SOCKET_ERROR)
        {
            bjlog.error("WSAEventSelect() error:", WSAGetLastError());
            exit(1);
        }
    }

    std::thread thr(&BJSocket::send_outgoing_routine, this);    

    while (sockets.size() > 0 && ahEvents.size() > 0)
    {
        DWORD dwEvent = WSAWaitForMultipleEvents(ahEvents.size(), &ahEvents[0], FALSE, WSA_INFINITE, FALSE);
        
        switch (dwEvent)
        {
        case WSA_WAIT_FAILED:
            bjlog.error("WSAEventSelect:", WSAGetLastError());
            break;
        case WSA_WAIT_TIMEOUT:
            bjlog.warning("Timeout");
            break;

        default:
        {            
            int i;
            for (i = 0; i < ahEvents.size(); ++i)
            {
                int someInt = WSAEnumNetworkEvents( sockets[i], ahEvents[i], &NetworkEvents);
                WSAResetEvent(ahEvents[i]);
                if (SOCKET_ERROR == someInt)
                {
                    bjlog.error("WSAEnumNetworkEvent:", WSAGetLastError());
                    NetworkEvents.lNetworkEvents = 0;
                }
                if (FD_ACCEPT & NetworkEvents.lNetworkEvents && is_server)
                {
                    AcceptConnection(i);
                }
                if (FD_READ & NetworkEvents.lNetworkEvents)
                {
                    ReceiveMessage(i);
                }
                if (FD_CLOSE & NetworkEvents.lNetworkEvents)
                {
                    CloseConnection(i);
                    // We've replaced socket[i] and event[i] with socket[last] and event[last], so we have to iterate [i] again
                    --i;
                    continue;
                }

            }
        }
            break;
        }

    }
    thr.join();
}


void BJSocket::send_outgoing_routine()
{
    char data[buffer_size];
    while (true)
    {

        BJMessage bjmessage = std::move(*outgoing.wait_and_pop().get());
        int message_length_remain = bjmessage.size();
        int iter = 0;
        while (message_length_remain >= 0)
        {
            if (message_length_remain > buffer_size)
            {
                memcpy_s(data, buffer_size, bjmessage.get_message().get() + iter * buffer_size, buffer_size);
            }
            else
            {
                memcpy_s(data, buffer_size, bjmessage.get_message().get() + iter * buffer_size, message_length_remain);
                memset(data + message_length_remain, message_delimiter, buffer_size - message_length_remain);
            }

            message_length_remain -= buffer_size;
            ++iter;
            int res = send(bjmessage.socket(), data, buffer_size, 0);
            if (res == SOCKET_ERROR) {
                bjlog.error("Couldn't send data to socket", bjmessage.socket(), "error:", WSAGetLastError());
            }
        }

    }

    /*
    transmitting message length
    while (true)
    {
        std::stringstream output;
        BJMessage bjmessage = std::move(*outgoing.wait_and_pop().get());
        UCHAR start_flag[5] = { 0xFF ,0xFF ,0xFF ,0xFF, '\0' };

        int x = bjmessage.size();
        UCHAR* msg_length = reinterpret_cast<UCHAR*>(&x);
        
        // TODO : check little endian and big endian
        output << start_flag << msg_length[0] << msg_length[1] << msg_length[2] << msg_length[3] << bjmessage.get_message_string();
        int res = send(bjmessage.socket(), output.str().c_str(), bjmessage.size() + sizeof(0xFFFF) + sizeof(bjmessage.size()), 0);
        if (res == SOCKET_ERROR) {
            bjlog.error("Couldn't send data to socket", bjmessage.socket(), "error:", WSAGetLastError());
        }        
    }*/
}

BJMessage BJSocket::Receive()
{
    return *incoming.wait_and_pop().get();
}

void BJSocket::ReceiveMessage(int client_number)
{

    /*
    transmitting message length
    UCHAR length_buffer[4];
    int counter = 0;
    while (length_buffer[0] != UCHAR_MAX || length_buffer[1] != UCHAR_MAX || length_buffer[2] != UCHAR_MAX || length_buffer[3] != UCHAR_MAX)
    {
        // until we receive start sequence 0xFFFF
        recv(sockets[client_number], (char*)length_buffer + (++counter % 4), 1, 0);
    }
    // receive 4 bytes, message length
    recv(sockets[client_number], (char*)length_buffer, 4, 0);
    int* c = reinterpret_cast<int*>(length_buffer);

    int message_length = *c;
    std::unique_ptr<char> data(new char[message_length]);
    recv(sockets[client_number], data.get(), message_length, 0);
    incoming.push(BJMessage(sockets[client_number], data.get(), message_length));
    */


    // we will receive messages buffer_size length, useful payload ends with message_delimiter
    char data[buffer_size + 1];
    data[buffer_size] = message_delimiter;
    char* end_pointer = NULL;

    int message_length = recv(sockets[client_number], data, buffer_size, 0);
    if (message_length > 0)
    {
        end_pointer = std::find(data, data + buffer_size, message_delimiter);
        incoming_message << std::string(data, end_pointer - data);
    }

    if (end_pointer != data + buffer_size)
    {
        incoming.push(BJMessage(sockets[client_number], incoming_message.str()));
        incoming_message.str(std::string());
    }
}
