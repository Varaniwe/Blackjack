#include "BJPack.h"



BJPack::BJPack()
{
    int curr_card = cards::two;
    do
    {
        int curr_suit = suits::hearts;
        do {
            pack.push_back(BJCard(static_cast<suits>(curr_suit), static_cast<cards>(curr_card)));
            ++curr_suit;
        } while (curr_suit <= suits::spades);
        
        ++curr_card;
    } while (curr_card <= cards::ACE);

}


BJPack::~BJPack()
{
}


bool BJPack::get_next_card(BJCard& next_card)
{
    if (pack.empty())
        return false;

    srand(std::time(NULL));
    int card_index = rand() % pack.size();
    next_card = BJCard(pack[card_index].get_suit(), pack[card_index].get_card());
    pack.erase(pack.begin() + card_index);
    return true;
}
