#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>

class Log
{
    std::ostream &stream;

public:
    enum LOG_LEVEL: uint8_t
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        LOG_LEVELS
    }; // enum LOG_LEVEL

    static const char *LEVEL_NAME[8];

    class Log_line
    {
        std::ostream &stream;
        const LOG_LEVEL level;
        const LOG_LEVEL line_level;

    public:
        Log_line(std::ostream &_stream, LOG_LEVEL _level, LOG_LEVEL _line_level);
        ~Log_line(void);

        template<typename TYPE>
        Log_line &operator<<(TYPE variable);
    }; // class Log_line

private:
    LOG_LEVEL level;

public:
    Log(std::ostream &_stream = std::cerr, LOG_LEVEL _level = INFO);

    void disable(void);
    void verbose(void);

    Log_line operator()(LOG_LEVEL level);

    template<typename TYPE>
    Log_line operator<<(TYPE variable);
}; // class Log

const char *Log::LEVEL_NAME[8] =
{
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "NOLOG",
};

inline
Log::Log(std::ostream &_stream, LOG_LEVEL _level)
:stream(_stream)
,level(_level)
{
}

inline
void Log::disable(void)
{
    level = LOG_LEVELS;
}

inline
void Log::verbose(void)
{
    level = DEBUG;
}

inline
Log::Log_line Log::operator()(Log::LOG_LEVEL _level)
{
    return Log_line{stream, level, _level};
}

template<typename TYPE>
inline
Log::Log_line Log::operator<<(TYPE variable)
{
    return Log_line{stream, level, level} << variable;
}

inline
Log::Log_line::Log_line(std::ostream &_stream, LOG_LEVEL _level, LOG_LEVEL _line_level)
:stream(_stream)
,level(_level)
,line_level(_line_level)
{
    if(line_level >= level)
        stream << Log::LEVEL_NAME[_line_level] << ": ";
}

Log::Log_line::~Log_line(void)
{
    if(line_level >= level)
        stream << "\n";
}

template<typename TYPE>
typename Log::Log_line::Log_line &Log::Log_line::operator<<(TYPE variable)
{
    if(line_level >= level)
        stream << variable;

    return *this;
}

#endif // __LOG_H__
