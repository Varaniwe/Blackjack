#pragma once
#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "BJLog.h"
#include "BJPack.h"
#include "IBJGameInterconnection.h"
#include "PlayerInfo.h"

class BJGame
{
private:
    IBJGameInterconnection* interconnection;
    std::map<SOCKET, PlayerInfo> players;
    std::vector<SOCKET> players_order;
    BJLog bjlog;
    BJPack pack;
    std::vector<BJCard> dealer_cards;
    bool game_is_processing;
    bool dealer_stealth;
    bool no_more_cards;
    std::mutex event_mutex;
    std::condition_variable event_cv;
    bool wait_for_bets;
    std::chrono::duration<int, std::ratio<1>> wait_for_bets_time;
    std::chrono::duration<int, std::ratio<1>> wait_for_card_time;

    void player_connect(SOCKET sckt);
    void player_disconnect(SOCKET sckt);
    void start_game();
    void process_game();
    void end_game();
    void offer_card(SOCKET sckt);
    void deal_card(SOCKET sckt);
    void deal_cards_to_all();
    std::string get_current_status();
    std::string get_current_status(SOCKET sckt);
    void ask_name(SOCKET sckt);
    void set_name(SOCKET sckt, std::string new_name);
    void ask_budget(SOCKET);
    void set_budget(SOCKET, int);
    void ask_bet(SOCKET, int);
    void set_bet(SOCKET, int);
    void dont_wait_input(SOCKET);
    void set_bet_wait_time(SOCKET, int);
    void set_card_wait_time(SOCKET, int);
    void notify_all(std::string);
    void send_status(SOCKET);
    void find_winner();
    int count_cards(SOCKET);
    int count_dealer_cards();
    void dealer_takes_card();
    void player_wins(SOCKET sckt, int num = 1, int den = 1);
    void player_loses(SOCKET sckt);
    void player_draw(SOCKET sckt);
    void execute_command(std::string cmnd);
    void execute_command(BJMessage);
public:
    BJGame();
    BJGame(IBJGameInterconnection* interconn);
    BJGame(const BJGame& other);
    ~BJGame();
    BJGame& operator=(const BJGame& other);


    void wait_event();
    void try_start_game();

};

