#pragma once

enum LogLevel
{
    debug = 0,
    info,
    warning,
    error
};

enum suits {
    default_suit,

    hearts,
    diamonds,
    clubs,
    spades
};

enum cards {
    default_card,

    two = 2,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    Jack,
    Queen,
    King,
    ACE
};

enum player_state {
    not_ready,
    ready,
    in_game
};