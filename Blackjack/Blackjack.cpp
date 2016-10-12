// Blackjack.cpp: определяет точку входа для консольного приложения.
//
#include <stdio.h>
#include <string>
#include <WS2tcpip.h>
#include <thread>

#include "BJServer.h"
#include "BJClient.h"
#include "BJGame.h"
#include "BJPlayer.h"

int main(int argc, char *argv[])
{
    std::locale::global(std::locale(""));
    BJSocket* some_socket = NULL;

    BJServer server;
    BJClient client;    
    BJGame game;
    BJPlayer player;
    bool is_server = true;
    if (argc > 1)
    {
        std::string par_1 = "-s";
        if (par_1.compare(argv[1]) == 0)
        {
            std::cout << "Server port " << argv[2] << std::endl;
            if (!server.BJServerInit(std::stoi(std::string(argv[2]))))
                return -1;
            some_socket = &(server);
            game = BJGame(some_socket);
        }
        else
        {
            is_server = false;
            std::cout << "Client" << std::endl;
            if (!client.BJConnect(argv[1], std::stoi(std::string(argv[2]))))
                return -1;
            some_socket = &(client);
            player = BJPlayer(some_socket);
        }
    }
    else
    {
        int default_port = 12346;
        std::cout << "Default server port " << default_port << std::endl;
        if (!server.BJServerInit(12346))
            return -1;
        some_socket = &(server);
        game = BJGame(some_socket);
    }

    std::thread network_thread(&BJSocket::BJWaitForEvents, some_socket);    
    std::thread player_events_thread;
    std::thread game_thread;

    if (is_server)
    {
        player_events_thread = std::thread(&BJGame::wait_event, &game);
        game_thread = std::thread(&BJGame::try_start_game, &game);
    }
    else
    {
        player_events_thread = std::thread(&BJPlayer::wait_event, &player);
    }

    network_thread.join();
    player_events_thread.join();
    if (is_server)
        game_thread.join();
    return 0;
}


