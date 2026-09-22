#include "rz.h"

namespace eqlib {

namespace {
NullLogger g_null_logger;
Logger* g_logger = &g_null_logger;
}

Logger* setGlobalLogger(Logger* logger) {
    Logger* old = g_logger;
    g_logger = logger ? logger : &g_null_logger;
    return old;
}

Logger* getGlobalLogger() {
    return g_logger;
}

}
