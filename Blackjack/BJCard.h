#pragma once
#include <string>

#include "Enums.h"

class BJCard
{
private:
    suits suit;
    cards card;

public:
    BJCard();
    BJCard(suits s, cards c);
    ~BJCard();

    suits get_suit();
    cards get_card();
    int get_calced_value(bool high);

    std::string get_string();
};


