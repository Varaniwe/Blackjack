#include "BJPlayer.h"

#undef max


BJPlayer::BJPlayer() : bjlog(std::cout)
{
    input_string = "invalid";
}



BJPlayer::~BJPlayer()
{
}

BJPlayer::BJPlayer(IBJGameInterconnection * interconn) : bjlog(std::cout)
{
    interconnection = interconn;
    bjlog.set_min_log_level(LogLevel::debug);
    input_string = "invalid";
}

BJPlayer & BJPlayer::operator=(const BJPlayer & other)
{
    if (this != &other)
    {
        std::unique_lock<std::mutex>  locker_2(event_mutex);
        interconnection = other.interconnection;
        bjlog = other.bjlog;
        waiting_for_input = other.waiting_for_input;
        input_string = other.input_string;
    }
    return *this;
}

int BJPlayer::count_cards()
{
    return 0;
}

void BJPlayer::wait_event()
{
    player_connect();
    std::thread input_thread(&BJPlayer::wait_input, this);
    
    while (true)
    {
        BJMessage msg = interconnection->Receive();
        execute_command(msg.get_message_string());
    }
    input_thread.join();
}

void BJPlayer::wait_input()
{
    while (true)
    {        
        std::cin >> input_string;
        std::cin.ignore(std::numeric_limits <std::streamsize>::max(), '\n');
        event_cv.notify_all();
        std::unique_lock<std::mutex> guard(event_mutex);
        if (!waiting_for_input)
        {
            if (input_string.compare("*") == 0)
                get_status();
            input_string = "invalid";
        }
    }
}

void BJPlayer::player_connect()
{
    std::string full_command = "player_connect:";
    interconnection->BJSendMessage(full_command);
}


void BJPlayer::offer_card()
{
    std::unique_lock<std::mutex> guard(event_mutex);
    waiting_for_input = true;

    std::string answer; 
    std::cout << "Will you take a card? (y/n)" << std::endl;
    event_cv.wait_for(guard, wait_for_card, [this, &answer]() {
        
        answer = input_string;
        std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
        input_string = "invalid";
        if (answer.compare("y") == 0 || answer.compare("n") == 0)
        {
            waiting_for_input = false;
            return true;
        }
        return false;
    });

    if (waiting_for_input)
    {
        waiting_for_input = false;
        return;
    }
    if (answer.compare("y") == 0)
    {
        ask_card();
        waiting_for_input = false;
    }
    else if (answer.compare("n") == 0)
    {
        refuse_card();
        waiting_for_input = false;
    }
    
}

void BJPlayer::ask_card()
{
    std::string full_command = "ask_card:";
    interconnection->BJSendMessage(full_command);
}

void BJPlayer::refuse_card()
{
    std::string full_command = "refuse_card:";
    interconnection->BJSendMessage(full_command);
}

void BJPlayer::make_bet()
{

    int bet = -1;
    std::cout << "Your bet: ";

    std::unique_lock<std::mutex> guard(event_mutex);
    waiting_for_input = true;
    

    event_cv.wait_for(guard, wait_for_bets_time, [this, &bet]() {
        
        if (sscanf_s(input_string.c_str(), "%d", &bet) == 0 || bet < 0)
        {
            if (input_string.compare("invalid") == 0)
                return false;

            std::cout << "Invalid input.  Try again: " << std::endl;
            return false;
        }
        input_string = "invalid";
        waiting_for_input = false;
        return true;
    });

    if (waiting_for_input)
    {
        waiting_for_input = false;
        return;
    }
    std::string full_command = "make_bet:" + std::to_string(bet);
    interconnection->BJSendMessage(full_command);
    waiting_for_input = false;
}

void BJPlayer::get_status()
{
    std::string full_command = "get_status:";
    interconnection->BJSendMessage(full_command);
}

void BJPlayer::set_name(std::string new_name)
{
    std::string full_command = "set_name:" + new_name;
    interconnection->BJSendMessage(full_command);
}

void BJPlayer::set_budget(int budget)
{
    std::string full_command = "set_budget:" + std::to_string(budget);
    interconnection->BJSendMessage(full_command);
}


void BJPlayer::execute_command(std::string cmnd)
{
    size_t delimiter_pos = cmnd.find(":");
    std::string command = cmnd.substr(0, delimiter_pos);
    std::string argument = "";
    if (delimiter_pos != std::string::npos)
    {
        argument = cmnd.substr(delimiter_pos + 1);
    }

    //bjlog.debug(command, argument);

    if (command.compare("ask_bet") == 0)
    {
        std::cout << "You have " << argument << "$" << std::endl;
        make_bet();
    }
    else if (command.compare("ask_name") == 0)
    {
        std::cout << "Your name: ";
        std::unique_lock<std::mutex> guard(event_mutex);
        std::string uname;
        waiting_for_input = true;
        event_cv.wait(guard, [this, &uname]() {
            uname = input_string; 
            input_string = "invalid";
            return uname.compare("invalid") != 0 || !waiting_for_input;
        });
        
        set_name(uname);
        waiting_for_input = false;
    }
    else if (command.compare("start_game") == 0)
    {
        std::cout << "Start game" << std::endl;
    }
    else if (command.compare("dont_wait_input") == 0)
    {

        std::unique_lock<std::mutex> guard(event_mutex);
        waiting_for_input = false;
        event_cv.notify_all();
    }
    else if (command.compare("offer_card") == 0)
    {
        offer_card();
    }
    else if (command.compare("player_wins") == 0)
    {
        std::cout << "Congratulations! You won " << std::stoi(argument) << "$" << std::endl;
    }
    else if (command.compare("player_loses") == 0)
    {
        std::cout << "You lost " << std::stoi(argument) << "$" << std::endl;
    }
    else if (command.compare("player_draw") == 0)
    {
        std::cout << "Draw" << std::endl;
    }
    else if (command.compare("send_status") == 0)
    {
        std::cout << argument << std::endl;
    }
    else if (command.compare("set_card_wait_time") == 0)
    {
        wait_for_card = std::chrono::duration<int, std::ratio<1>>(std::stoi(argument));
    }
    else if (command.compare("set_bet_wait_time") == 0)
    {
        wait_for_bets_time = std::chrono::duration<int, std::ratio<1>>(std::stoi(argument));        
    }
    else if (command.compare("ask_budget") == 0)
    {
        std::cout << "Your budget: ";
        std::unique_lock<std::mutex> guard(event_mutex);
        int budget;
        waiting_for_input = true;
        event_cv.wait(guard, [this, &budget]() {
            
            if (sscanf_s(input_string.c_str(), "%d", &budget) == 0)
                return false;
            input_string = "invalid";
            if (budget < 0)
            {
                std::cout << "Invalid input.  Try again: " << std::endl;
                return false;
            }
            waiting_for_input = false;
            return true;
        });            

        set_budget(budget);
        waiting_for_input = false;
    }
    else
    {
        std::cout << cmnd << std::endl;
    }
}



