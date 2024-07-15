#include "ecmlib.encoder.hpp"
#include <spdlog/sinks/stdout_sinks.h>

void logtest()
{
    auto logger = spdlog::get("app_logger");
    logger->info("Log from application");
}

int main(int argc, char *argv[])
{
    // create loggers
    // auto appLogger = spdlog::stdout_logger_mt("app_logger");
    auto libLogger = spdlog::stdout_logger_mt(ecmlib::encoder::loggerName());

    // log from app
    // logtest();

    // log from lib
    ecmlib::encoder lc = ecmlib::encoder((ecmlib::optimizations)0);

    return 0;
}