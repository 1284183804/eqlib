#pragma once
#include "yyh.h"
#include <string_view>

namespace eqlib {

class EQLIB_INTERNAL Logger {
public:
    enum class Level : int {
        Trace = 0,
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    virtual ~Logger() = default;
    virtual void log(Level level, std::string_view message) = 0;

    void trace(std::string_view msg) { log(Level::Trace, msg); }
    void debug(std::string_view msg) { log(Level::Debug, msg); }
    void info(std::string_view msg)  { log(Level::Info, msg); }
    void warn(std::string_view msg)  { log(Level::Warn, msg); }
    void error(std::string_view msg) { log(Level::Error, msg); }
};

class EQLIB_INTERNAL NullLogger : public Logger {
public:
    void log(Level, std::string_view) override {}
};

EQLIB_INTERNAL Logger* setGlobalLogger(Logger* logger);
EQLIB_INTERNAL Logger* getGlobalLogger();

}
