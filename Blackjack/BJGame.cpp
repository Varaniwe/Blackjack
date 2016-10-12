#include "BJGame.h"

BJGame::BJGame(): bjlog(std::cout)
{
}

BJGame::BJGame(IBJGameInterconnection * interconn) : bjlog(std::cout)
{
    interconnection = interconn;
    bjlog.set_min_log_level(LogLevel::debug);
    game_is_processing = false;
    wait_for_bets = false;
    wait_for_bets_time = std::chrono::duration<int, std::ratio<1>>(10);
    wait_for_card_time = std::chrono::duration<int, std::ratio<1>>(10);
}

BJGame::BJGame(const BJGame & other) : bjlog(std::cout)
{
    interconnection = other.interconnection;
    players = other.players;
    players_order = other.players_order;
    bjlog = other.bjlog;
    pack = other.pack;
    game_is_processing = other.game_is_processing;
    wait_for_bets = other.wait_for_bets;
    wait_for_card_time = other.wait_for_card_time;
    wait_for_bets_time = other.wait_for_bets_time;
}


BJGame::~BJGame()
{
}

BJGame& BJGame::operator=(const BJGame& other)
{
    if (this != &other)
    {
        std::unique_lock<std::mutex>  locker(event_mutex);
        interconnection = other.interconnection;
        players = other.players;
        players_order = other.players_order;
        bjlog = other.bjlog;
        pack = other.pack;
        game_is_processing = other.game_is_processing;
        wait_for_bets = other.wait_for_bets;
        wait_for_card_time = other.wait_for_card_time;
        wait_for_bets_time = other.wait_for_bets_time;
    }
    return *this;
}


void BJGame::player_connect(SOCKET sckt)
{
    std::lock_guard<std::mutex> guarder(event_mutex);

    players.insert(std::pair<SOCKET, PlayerInfo>(sckt, PlayerInfo()));
    players[sckt].username = "Unnamed";
    players[sckt].money = -1;
    players[sckt].bet = -1;
    players[sckt].state = player_state::not_ready;
    players_order.push_back(sckt); 
    set_card_wait_time(sckt, wait_for_card_time.count());
    set_bet_wait_time(sckt, wait_for_bets_time.count());
    ask_name(sckt);
}

void BJGame::player_disconnect(SOCKET sckt)
{
    std::lock_guard<std::mutex> guarder(event_mutex);

    bjlog.write("Player", players[sckt].username, "disconnected");
    players.erase(sckt);    
}

void BJGame::wait_event()
{
    while (true)
    {
        BJMessage msg = interconnection->Receive();
        execute_command(msg);
    }

}

void BJGame::try_start_game()
{
    while (true)
    {
        bool can_begin = false;
        std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1>>(1));
        
        {
            std::lock_guard<std::mutex> guarder(event_mutex);
            for (auto it = players.begin(); it != players.end(); ++it) 
            {
                if ((*it).second.money > 0 && (*it).second.state == player_state::ready)
                    can_begin = true;
                else if ((*it).second.money == 0)
                {
                    BJMessage msg((*it).first, "You don't have money");
                    interconnection->BJSendMessage(msg);
                    (*it).second.state == player_state::not_ready;
                }

            }
            can_begin = can_begin && !game_is_processing;
        }
            

        if (can_begin)
        {            
            // wait for 3 seconds
            std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1>>(3));
            start_game();
        }
    }    
}

void BJGame::start_game()
{
    std::string full_command = "start_game:";
    {
        std::lock_guard<std::mutex> guarder(event_mutex);

        for (auto it = players.begin(); it != players.end(); ++it)
        {
            if ((*it).second.state == player_state::not_ready)
                continue;

            game_is_processing = true;
            (*it).second.state = player_state::in_game;
            BJMessage msg((*it).first, full_command);
            interconnection->BJSendMessage(msg);
        }
        if (!game_is_processing)
        {
            end_game();
            return;
        }
    }

    process_game();
}

void BJGame::process_game()
{

    {
        std::lock_guard<std::mutex> guarder(event_mutex); 
        wait_for_bets = true;
        for (auto pl : players)
        {
            if (pl.second.state == player_state::in_game)
                ask_bet(pl.first, pl.second.money);
        }
    }
    
    {
        // wait until all players will make bet or wait_for_bets_time pass
        bjlog.debug("Wait for bets");
        notify_all("Wait for bets");
        bool can_continue = false;
        {
            std::unique_lock<std::mutex> guarder(event_mutex);
            dealer_stealth = true;

            // wait for somebody will make bet
            event_cv.wait_for(guarder, wait_for_bets_time,
                [this, &can_continue]() {
                for (auto pl_o : players_order)
                {
                    if (players.count(pl_o) == 0)
                        continue;
                    if (players[pl_o].state == player_state::in_game)
                    {
                        if (players[pl_o].bet < 0)
                        {
                            return false;
                        }
                        can_continue = true;
                    }                    
                }
                return can_continue;
            });

            wait_for_bets = false;
            // make players who didn't make bet waiting for next round 
            if (can_continue)
            {
                for (auto pl_o : players_order)
                {
                    if (players.count(pl_o) == 0)
                        continue;
                    if (players[pl_o].state == player_state::in_game && players[pl_o].bet < 0)
                    {
                        dont_wait_input(pl_o);
                        players[pl_o].state = player_state::ready;
                        BJMessage msg(pl_o, "Timeout");
                        interconnection->BJSendMessage(msg);
                    }
                }
            }
        }
        
        // if somebody made bet deal cards to players in game
        if (can_continue)
        {
            bjlog.debug("Bets done");
            deal_cards_to_all();
        }
        else
        {
            bjlog.debug("Timeout");
            end_game();
            return;
        }

        // offer cards in order
        {
            std::unique_lock<std::mutex> guarder(event_mutex);
            for (auto pl_o : players_order)
            {
                if (players.count(pl_o) == 0)
                    continue;
                no_more_cards = false;
                if (players[pl_o].state != player_state::in_game)
                    continue;
                offer_card(pl_o);
                event_cv.wait_for(guarder, wait_for_card_time, [this]() {
                    return no_more_cards; 
                });
            }
        }

        std::string curr_status = get_current_status();
        notify_all(curr_status);
        find_winner();
        end_game();
    }
}

void BJGame::end_game()
{    
    dealer_cards.clear();
    {
        std::lock_guard<std::mutex> guarder(event_mutex);
        players_order.clear();
        for (auto it = players.begin(); it != players.end(); ++it)
        {
            if ((*it).second.state == player_state::in_game)
            {
                (*it).second.state = player_state::ready;
            }
            (*it).second.cards.clear();

            dont_wait_input((*it).first);
            players_order.push_back((*it).first);
        }

        game_is_processing = false;
    }
}

void BJGame::offer_card(SOCKET sckt)
{
    std::string full_command = "offer_card:";
    BJMessage msg(sckt, full_command);
    interconnection->BJSendMessage(msg);
}

void BJGame::deal_card(SOCKET sckt)
{
    BJCard card;
    
    if (!pack.get_next_card(card))
    {
        bjlog.write("Pack is empty");
        notify_all("Pack is empty. New pack");
        pack = BJPack();
        pack.get_next_card(card);
    }
    int card_sum;
    {
        std::lock_guard<std::mutex> guarder(event_mutex);
        players[sckt].cards.push_back(card);
        card_sum = count_cards(sckt);
        bjlog.write(players[sckt].username, "takes", card.get_string());
    }
    notify_all(get_current_status(sckt));
    if (card_sum < 21)
    {
        offer_card(sckt);
    }
    else
    {
        std::lock_guard<std::mutex> guarder(event_mutex);
        no_more_cards = true;
        event_cv.notify_all();
    }
}

void BJGame::deal_cards_to_all()
{
    BJCard card;
    for (int i = 0; i < 2; ++i)
    {
        if (!pack.get_next_card(card))
        {
            bjlog.write("Pack is empty");
            notify_all("Pack is empty. New pack");
            pack = BJPack(); 
            pack.get_next_card(card);
        } 
        bjlog.write("Dealer takes", card.get_string());
        dealer_cards.push_back(card);            
        

        std::lock_guard<std::mutex> guarder(event_mutex);
        for (auto pl_n : players_order)
        {
            if (players.count(pl_n) == 0)
                continue;

            if (players[pl_n].state != player_state::in_game)
                continue;
            if (!pack.get_next_card(card))
            {
                bjlog.write("Pack is empty");
                pack = BJPack();
                pack.get_next_card(card);
            }
            bjlog.write(players[pl_n].username, "takes", card.get_string());
            players[pl_n].cards.push_back(card);
        }
    }

    std::string curr_status = get_current_status();
    notify_all(curr_status);    
}

std::string BJGame::get_current_status()
{
    std::ostringstream stringstream;

    stringstream << "dealer:\n";
    int dealer_cards_count = dealer_cards.size();
    std::string separator = "";
    if (dealer_stealth)
    {
        dealer_cards_count--; 
        stringstream << separator << "*******";
        separator = ", ";
    }
    for (int i = 0 ; i < dealer_cards_count; ++i)
    {
        stringstream << separator << dealer_cards[i].get_string() ;
        separator = ", ";
    }
    stringstream << "\n";

    std::lock_guard<std::mutex> guarder(event_mutex);

    for (auto i : players_order)
    {
        if (players.count(i) == 0)
            continue;

        if (players[i].state != player_state::in_game)        
            continue;
        
        stringstream << players[i].username << " (bet: " << players[i].bet << "$)\n";
        separator = "";
        for (BJCard card : players[i].cards)
        {
            stringstream << separator << card.get_string();
            separator = ", "; 
        }
        stringstream << "\n";
    }
    return stringstream.str();
}

std::string BJGame::get_current_status(SOCKET sckt)
{
    std::ostringstream stringstream;

    std::lock_guard<std::mutex> guarder(event_mutex);

    if (!(players[sckt].state == player_state::in_game))
    {
        if (players[sckt].state == player_state::ready)
            stringstream << players[sckt].username << " is waiting";

        return stringstream.str();
    }

    stringstream << players[sckt].username << " (bet: " << players[sckt].bet << "$)\n";
    std::string separator = "";
    for (BJCard card : players[sckt].cards)
    {
        stringstream << separator << card.get_string();
        separator = ", ";
    }
    stringstream << "\n";
    return stringstream.str();
}

void BJGame::ask_name(SOCKET sckt)
{
    std::string full_command = "ask_name:";
    BJMessage msg(sckt, full_command);
    interconnection->BJSendMessage(msg);
}

void BJGame::set_name(SOCKET sckt, std::string new_name)
{

    std::lock_guard<std::mutex> guarder(event_mutex);

    players[sckt].username = new_name;
    // ask budget if default
    if (players[sckt].money == -1)
    {
        ask_budget(sckt);
    }
}

void BJGame::ask_budget(SOCKET sckt)
{
    std::string full_command = "ask_budget:";
    BJMessage msg(sckt, full_command);
    interconnection->BJSendMessage(msg);
}

void BJGame::set_budget(SOCKET sckt, int budget)
{
    std::lock_guard<std::mutex> guarder(event_mutex);

    players[sckt].money = budget;
    players[sckt].state = player_state::ready;
}

void BJGame::ask_bet(SOCKET sckt, int money)
{
    std::string full_command = "ask_bet:" + std::to_string(money);
    BJMessage msg(sckt, full_command);
    interconnection->BJSendMessage(msg);
}

void BJGame::set_bet(SOCKET sckt, int bet)
{
    std::string notification;

    std::lock_guard<std::mutex> guarder(event_mutex);
    if (wait_for_bets)
    {
        if (bet > players[sckt].money)
        {
            notification = "You don't have so much money";
            ask_bet(sckt, players[sckt].money);
        }
        else
        {
            players[sckt].bet = bet;
            notification = "Your bet is " + std::to_string(bet) + "$";
            bjlog.write(players[sckt].username, "made bet", bet);
        }
        BJMessage msg(sckt, notification);
        interconnection->BJSendMessage(msg);
        event_cv.notify_all();        
    }
    
}

void BJGame::dont_wait_input(SOCKET sckt)
{
    BJMessage msg(sckt, "dont_wait_input:");
    interconnection->BJSendMessage(msg);
}

void BJGame::set_bet_wait_time(SOCKET sckt, int time)
{
    std::string cmd = "set_bet_wait_time:" + std::to_string(wait_for_bets_time.count());
    BJMessage msg(sckt, cmd);
    interconnection->BJSendMessage(msg);
}

void BJGame::set_card_wait_time(SOCKET sckt, int time)
{
    std::string cmd = "set_card_wait_time:" + std::to_string(wait_for_card_time.count());
    BJMessage msg(sckt, cmd);
    interconnection->BJSendMessage(msg);
}

void BJGame::notify_all(std::string message)
{
    std::lock_guard<std::mutex> guarder(event_mutex);
    for (auto s : players)
    {
        if (s.second.state == player_state::not_ready)
            continue;
        BJMessage msg(s.first, message);
        interconnection->BJSendMessage(msg);
    }
}

void BJGame::send_status(SOCKET sckt)
{    
    BJMessage msg(sckt, get_current_status(sckt));
    interconnection->BJSendMessage(msg);
}



void BJGame::find_winner()
{
    int dealer_sum = count_dealer_cards();

    std::lock_guard<std::mutex> guarder(event_mutex);
    if (!game_is_processing)
        return;
    for (auto pl_o : players_order)
    {

        if (players.count(pl_o) == 0)
            continue;

        if (players[pl_o].state != player_state::in_game)
            continue;        
        
        int cards_sum = count_cards(pl_o);
        if (cards_sum > 21)
        {
            player_loses(pl_o);
            bjlog.write(players[pl_o].username, "loses");
        }
        else if ((cards_sum > dealer_sum && dealer_sum <= 21) || dealer_sum > 21)
        {
            player_wins(pl_o, 1, 1);
            bjlog.write(players[pl_o].username, "wins");
        }
        else if (cards_sum < dealer_sum)
        {
            player_loses(pl_o);
            bjlog.write(players[pl_o].username, "loses");
        }
        else
        {
            player_draw(pl_o);
            bjlog.write(players[pl_o].username, "draw");
        }
    }
}


int BJGame::count_cards(SOCKET sckt)
{
    int res = 0;

    for (auto c : players[sckt].cards)
    {
        res += c.get_calced_value(true);
    }
    if (res > 21)
    {
        res = 0;
        for (auto c : players[sckt].cards)
        {
            res += c.get_calced_value(false);
        }
    }
    bjlog.write(players[sckt].username," score is", res);
    return res;
}

int BJGame::count_dealer_cards()
{
    int res = 0;
    bool high_value = true;
    {
        std::lock_guard<std::mutex> guarder(event_mutex);
        if (!game_is_processing)
            return 0;
        dealer_stealth = false;
    }

    // считаем первоначальное значение
    for (auto c : dealer_cards)
    {
        res += c.get_calced_value(high_value);
    }
    bool need_more_cards = res < 16;
    while (true)
    {
        // если меньше 16, берем еще
        if (need_more_cards)
            dealer_takes_card();

        res = 0;
        BJCard card; 

        for (auto c : dealer_cards)
        {
            res += c.get_calced_value(high_value);
        }
        std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1>>(1));        

        if (res >= 16 && res <= 21)
        {
            bjlog.write("Dealer's sum is", res); 
            std::string curr_status = get_current_status();
            notify_all(curr_status);
            return res;
        }
        // если перебор, считаем по минимуму
        else if (res > 21 )
        {
            // сли уже считали по минимуму, возвращаем
            if (!high_value)
                return res;
            high_value = false;
            need_more_cards = false;
        }
        // если нет 16, берем еще карту
        else
        {
            need_more_cards = true;
        }
        
    }    
}

void BJGame::dealer_takes_card()
{
    BJCard card;
    if (pack.get_next_card(card))
    {
        dealer_cards.push_back(card);
        std::string curr_status = "Dealer takes one more card\n" + get_current_status();
        notify_all(curr_status);
    }
    else
    {
        bjlog.write("Pack is empty");
        notify_all("Pack is empty. New pack");
        pack = BJPack();
        dealer_takes_card();
    }
}

void BJGame::player_wins(SOCKET sckt, int num, int den)
{
    int prize = players[sckt].bet * num / den;
    players[sckt].money += prize;
    players[sckt].bet = -1;

    std::string command = "player_wins:" + std::to_string(prize);
    BJMessage msg(sckt, command);
    interconnection->BJSendMessage(msg);
}

void BJGame::player_loses(SOCKET sckt)
{
    std::string command = "player_loses:" + std::to_string(players[sckt].bet);
    BJMessage msg(sckt, command);
    interconnection->BJSendMessage(msg);

    players[sckt].money -= players[sckt].bet;
    players[sckt].bet = -1;
}

void BJGame::player_draw(SOCKET sckt)
{
    std::string command = "player_draw:";
    BJMessage msg(sckt, command);
    interconnection->BJSendMessage(msg);

    players[sckt].bet = -1;
}


void BJGame::execute_command(std::string cmnd)
{
    throw std::exception("not implemented");
}

void BJGame::execute_command(BJMessage msg)
{
    std::string cmnd = msg.get_message_string();
    size_t delimiter_pos = cmnd.find(":");
    std::string command = cmnd.substr(0, delimiter_pos);
    std::string argument = cmnd.substr(delimiter_pos + 1);
        
    bjlog.debug(players[msg.socket()].username, command, argument);

    if (command.compare("make_bet") == 0)
    {
        int bet = std::stoi(argument);
        set_bet(msg.socket(), bet);
    }
    else if (command.compare("set_name") == 0)
    {
        set_name(msg.socket(), argument);
    }
    else if (command.compare("set_budget") == 0)
    {
        set_budget(msg.socket(), std::stoi(argument));    
    }
    else if (command.compare("get_status") == 0)
    {
        send_status(msg.socket());
    }
    else if (command.compare("player_connect") == 0)
    {
        player_connect(msg.socket());
    }
    else if (command.compare("player_disconnect") == 0)
    {
        player_disconnect(msg.socket());
    }
    else if (command.compare("ask_card") == 0)
    {
        deal_card(msg.socket());
    }
    else if (command.compare("refuse_card") == 0)
    {
        std::lock_guard<std::mutex> guarder(event_mutex);
        no_more_cards = true;
        event_cv.notify_all();
    }
    else
    {
        bjlog.debug("unknown command");
    }
}
