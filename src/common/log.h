#ifndef LOG_H
#define LOG_H

#include <iostream>

enum LogLevel{
    LOG_LEVEL_EMERG = 0,
    LOG_LEVEL_CRIT,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARINING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
};

class Log{
public:
    static Log* GetInstance(){
        static Log *_log = NULL;
        if(!_log) {
            _log = new Log();
        }
        return _log;
    }

    Log(enum LogLevel level = LOG_LEVEL_DEBUG):_level(level) {

    }

    void Printf(enum LogLevel level, const char *file, const char *fun, int l, const char *fmt, ...);
private:
    enum LogLevel _level;
};

#define LOG_EMERG(...) Log::GetInstance()->Printf(LOG_EMERG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_CRIT(...) Log::GetInstance()->Printf(LOG_LEVEL_CRIT, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_ERR(...) Log::GetInstance()->Printf(LOG_LEVEL_ERR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_WARNING(...) Log::GetInstance()->Printf(LOG_LEVEL_WARINING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_INFO(...) Log::GetInstance()->Printf(LOG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_DEBUG(...) Log::GetInstance()->Printf(LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);



#endif
