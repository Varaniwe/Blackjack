#include "BJCard.h"


BJCard::BJCard()
{
    suit = suits::default_suit;
    card = cards::default_card;
}

BJCard::BJCard(suits s, cards c)
{
    suit = s;
    card = c;
}


BJCard::~BJCard()
{
}

suits BJCard::get_suit()
{
    return suit;
}

cards BJCard::get_card()
{
    return card;
}

int BJCard::get_calced_value(bool high)
{
    switch (card)
    {
    case two:
    case three:
    case four:
    case five:
    case six:
    case seven:
    case eight:
    case nine:
    case ten:
        return (int)card;
    case Jack:
    case Queen:
    case King:
        return 10;
    case ACE:
        return high ? 11 : 1;
        break;
    default:
        throw std::exception("Unknown card value");
    }
}

std::string BJCard::get_string()
{
    std::string res = "";
    switch (card)
    {
    case two:
    case three:
    case four:
    case five:
    case six:
    case seven:
    case eight:
    case nine:
    case ten:
        res = std::to_string(card);
        break;
    case Jack:
        res = "Jack";
        break;
    case Queen:
        res = "Queen";
        break;
    case King:
        res = "King";
        break;
    case ACE:
        res = "ACE";
        break;
    default:
        throw std::exception("Unknown card value");
    }
    res += " ";
    switch (suit)
    {
    case hearts:
        res += "hearts";
        break;
    case diamonds:
        res += "diamonds";
        break;
    case clubs:
        res += "clubs";
        break;
    case spades:
        res += "spades";
        break;
    default:
        throw std::exception("Unknown card suit");
    }
    return res;
}
