#include "BJLog.h"


BJLog::BJLog(std::ostream& p_stream)
{
    stream = &p_stream;
    curr_log_level = LogLevel::info;
    min_log_level = LogLevel::info;
}

BJLog::~BJLog()
{

}

void BJLog::set_curr_log_level(LogLevel loglvl)
{
    curr_log_level = loglvl;
}

void BJLog::set_min_log_level(LogLevel loglvl)
{
    min_log_level = loglvl;
}
