#include "BJSocket.h"

BJSocket::BJSocket() : bjlog(std::cout)
{
    SOCKET own_socket = INVALID_SOCKET;
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

    std::thread thr(
        [this]() 
    {
        char data[buffer_size];
        while (true)
        {                    
            BJMessage bjmessage = std::move(*outgoing.wait_and_pop().get());
            int message_length_remain = bjmessage.size();
            int iter = 0;
            while (message_length_remain > 0)
            {
                if (message_length_remain > buffer_size)
                {
                    memcpy_s(data, buffer_size, bjmessage.get_message().get() + iter * buffer_size, buffer_size);
                }
                else
                {
                    memcpy_s(data, buffer_size, bjmessage.get_message().get() + iter * buffer_size, message_length_remain);
                }
                
                if (message_length_remain / buffer_size < 1)
                {
                    memset(data + message_length_remain, '\0', buffer_size - message_length_remain);
                }
                message_length_remain -= buffer_size;
                ++iter;
                //int res = send(bjmessage.socket(), bjmessage.get_message().get(), bjmessage.size(), 0);
                //bjlog.debug("sending", data);
                //std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1, 1000>>(10));
                int res = send(bjmessage.socket(), data, buffer_size, 0);
                if (res == SOCKET_ERROR) {
                    bjlog.error("Couldn't send data to socket", bjmessage.socket(), "error:", WSAGetLastError());
                }
            }
            
        }        
    });
    

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
            std::lock_guard<std::mutex> locker(sockets_mutex);

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
}



BJMessage BJSocket::Receive()
{
    return *incoming.wait_and_pop().get();
}

void BJSocket::ReceiveMessage(int client_number)
{

    char data[buffer_size + 1];
    std::stringstream message;
    data[buffer_size] = '\0';
    int message_length = 0;
    char* end_pointer = NULL;

    do
    {
        message_length += recv(sockets[client_number], data, buffer_size, 0);
        end_pointer = std::find(data, data + buffer_size, '\0');
        message << std::string(data);
    } while (end_pointer == data + buffer_size);

    if (message_length > 0)
    {
        incoming.push(BJMessage(sockets[client_number], message.str()));
    }
}
