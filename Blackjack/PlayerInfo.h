#pragma once
#include <string>
#include <vector>

#include "BJCard.h"
class PlayerInfo
{
public:
    PlayerInfo();
    ~PlayerInfo();

    std::vector<BJCard> cards;
    int money;
    int bet;
    std::string username;
    player_state state;
};

