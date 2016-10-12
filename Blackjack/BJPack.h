#pragma once
#include <vector>
#include <random>
#include <ctime>

#include "BJCard.h"
class BJPack
{
private:
    std::vector<BJCard> pack;
public:
    BJPack();
    ~BJPack();

    bool get_next_card(BJCard&);
};

