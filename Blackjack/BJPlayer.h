#pragma once
#include <algorithm>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "IBJGameInterconnection.h"
#include "BJCard.h"
#include "BJLog.h"

class BJPlayer
{
private:
    IBJGameInterconnection* interconnection;

    //thread_pool threads;
    std::chrono::duration<int, std::ratio<1>> wait_for_bets_time;
    std::chrono::duration<int, std::ratio<1>> wait_for_card;
    std::string input_string;
    bool waiting_for_input;
    std::mutex event_mutex;
    std::condition_variable event_cv;
    BJLog bjlog;

    void wait_input();
    void player_connect();
    void start_game();
    void offer_card();
    void ask_card();
    void refuse_card();
    void enough_cards();
    void make_bet();
    void get_status();
    int count_cards();
    void set_name(std::string);
    void set_budget(int budget);
    void execute_command(std::string cmnd);
public:
    BJPlayer();
    ~BJPlayer();

    BJPlayer(IBJGameInterconnection* interconn);
    BJPlayer& operator=(const BJPlayer& other);

    void wait_event();
    


};

