#pragma once
#include <string>
#include <sstream>
#include <mutex>

#include "Enums.h"


class BJLog 
{
private:
    std::ostream* stream;
    LogLevel curr_log_level;
    LogLevel min_log_level;
    std::recursive_mutex log_mutex;

    // почему нельзя делать нешаблонную функцию
    template<typename First>
    std::string get_current_time(First f)
    {
            time_t  timev;
            time(&timev);
            tm my_tm;
            auto errnono = localtime_s(&my_tm, &timev);
            std::ostringstream str_stream;

            int date_str_size = 10 + 1;
            std::unique_ptr<char> date_str(new char[date_str_size]);
            sprintf_s((char* const)date_str.get(), date_str_size, "%02d/%02d/%04d", my_tm.tm_mday, my_tm.tm_mon + 1, my_tm.tm_year + 1900);

            str_stream << date_str.get() << " ";
            //str_stream << my_tm.tm_mday << '/' << my_tm.tm_mon + 1 << '/' << my_tm.tm_year + 1900 << " ";

            int time_str_size = 8 + 1;
            std::unique_ptr<char> time_str(new char[time_str_size]);
            sprintf_s((char* const)time_str.get(), time_str_size, "%02d:%02d:%02d", my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec);
            str_stream << time_str.get() << '\n';
            return str_stream.str();
    }

    template<typename First>
    void write_without_time(First argf)
    {
        *stream << argf << " ";
    }
    
    template<typename First, typename ... Args>
    void write_without_time(First argf, Args ... args)
    {
        write_without_time(argf);
        write_without_time(args...);
    }
public:
    BJLog(std::ostream& p_stream);
    ~BJLog();    

    template<typename First>
    void write(First argf)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > curr_log_level)
            return;

        switch (curr_log_level)
        {
        case LogLevel::info:
            write_without_time("INFO:");
            break;
        case LogLevel::warning:
            write_without_time("WARNING:");
            break;
        case LogLevel::error:
            write_without_time("ERROR:");
            break;
        default:
            break;
        }

        write_without_time(argf);
                
        //write_without_time(my_tm.tm_mday + 1, '/', my_tm.tm_mon + 1, '/', my_tm.tm_year + 1900, " ", my_tm.tm_hour, ':', my_tm.tm_min, ":", my_tm.tm_sec, '\n');
        *stream << get_current_time(1);
    }

    template<typename First, typename ... Args>
    void write(First argf, Args ... args)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > curr_log_level)
            return;

        switch (curr_log_level)
        {
        case LogLevel::info:
            write_without_time("INFO:");
            break;
        case LogLevel::warning:
            write_without_time("WARNING:");
            break;
        case LogLevel::error:
            write_without_time("ERROR:");
            break;
        default:
            break;
        }

        write_without_time(argf);
        write_without_time(args...);
        
        //write_without_time(my_tm.tm_mday + 1, '/', my_tm.tm_mon + 1, '/', my_tm.tm_year + 1900, " ", my_tm.tm_hour, ':', my_tm.tm_min, ":", my_tm.tm_sec, '\n');
        *stream <<get_current_time(1);
    }
        

    template<typename First>
    void debug(First argf)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::debug)
            return;

        write_without_time("DEBUG:");
        write_without_time(argf);
        *stream << get_current_time(1);
    }


    template<typename First, typename ... Args>
    void debug(First argf, Args ... args)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::debug)
            return;

        write_without_time("DEBUG:");
        write_without_time(argf);
        write_without_time(args...);
        *stream << get_current_time(1);
    }
    
    template<typename First>
    void error(First argf)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::error)
            return;

        write_without_time("ERROR:");
        write_without_time(argf);
        *stream << get_current_time(1);
    }


    template<typename First, typename ... Args>
    void error(First argf, Args ... args)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::error)
            return;

        write_without_time("ERROR:");
        write_without_time(argf);
        write_without_time(args...);
        *stream << get_current_time(1);
    }

    template<typename First>
    void warning(First argf)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::warning)
            return;

        write_without_time("WARNING:");
        write_without_time(argf);
        *stream << get_current_time(1);
    }


    template<typename First, typename ... Args>
    void warning(First argf, Args ... args)
    {
        std::lock_guard<std::recursive_mutex> locker(log_mutex);
        if (min_log_level > LogLevel::warning)
            return; 

        write_without_time("WARNING:");
        write_without_time(argf);
        write_without_time(args...);
        *stream << get_current_time(1);
    }

    BJLog& operator<< (LogLevel loglvl)
    {
        curr_log_level = loglvl;
        return *this;
    }

    template<typename First>
    BJLog& operator<<(First log_text)
    {
        if (min_log_level > curr_log_level)
            return;

        write_without_time(log_text);
        return *this;
    }    

    void set_curr_log_level(LogLevel loglvl);
    void set_min_log_level(LogLevel loglvl);

    BJLog & BJLog::operator=(const BJLog & other)
    {
        if (this != &other)
        {
            std::unique_lock<std::recursive_mutex>  locker_2(log_mutex);
            stream = other.stream;
            curr_log_level = other.curr_log_level;
            min_log_level = other.min_log_level;
        }
        return *this;
    }
};


