#include "ecmlib.encoder.hpp"
#include <spdlog/sinks/stdout_sinks.h>
#include <fstream>
#include <iostream>
#include <string>

struct testData
{
    std::string file;
    uint8_t type;
};

int main(int argc, char *argv[])
{
    // create loggers
    auto appLogger = spdlog::stdout_logger_mt("app_logger");
    auto libLogger = spdlog::stdout_logger_mt(ecmlib::encoder::loggerName());
    libLogger->set_level(spdlog::level::trace);

    // Input buffer
    std::vector<char> buffer(2352);

    // Initialize the ecmlib class
    ecmlib::encoder ecmEncoder = ecmlib::encoder((ecmlib::optimizations)0);

    // Checking the mode of the following files
    std::vector<testData> filesToCheck = {
        {"mode1_gap.bin", 4},
        {"mode2.bin", 6}};

    for (uint8_t i = 0; i < filesToCheck.size(); i++)
    {
        std::ifstream inFile(filesToCheck[i].file);
        if (inFile.is_open())
        {
            appLogger->info("Reading the file \"{}\".", filesToCheck[i].file);
            inFile.read(buffer.data(), buffer.size());
            inFile.close();
        }
        else
        {
            appLogger->error("There was an error reading the input file \"{}\".", filesToCheck[i].file);
            return 1;
        }

        ecmEncoder.load(buffer.data(), buffer.size());
        // appLogger->info("The detected sector type is {}.", ecmEncoder.loggerName());
    }

    return 0;
}