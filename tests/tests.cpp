#include "ecmlib.encoder.hpp"
#include <spdlog/sinks/stdout_sinks.h>
#include <fstream>
#include <iostream>
#include <string>

struct testData
{
    std::string file;
    ecmlib::sector_type type;
};

int main(int argc, char *argv[])
{
    // create loggers
    auto appLogger = spdlog::stdout_logger_mt("app_logger");
    auto libLogger = spdlog::stdout_logger_mt(ecmlib::encoder::logger_name());
    libLogger->set_level(spdlog::level::trace);

    // Input buffer
    std::vector<char> buffer(2352);

    // Initialize the ecmlib class
    ecmlib::encoder ecmEncoder = ecmlib::encoder((ecmlib::optimizations)0);

    // Checking the mode of the following files
    std::vector<testData> filesToCheck = {
        {"cdda.bin", ecmlib::ST_CDDA},
        {"cdda_gap.bin", ecmlib::ST_CDDA_GAP},
        {"mode1.bin", ecmlib::ST_MODE1},
        {"mode1_gap.bin", ecmlib::ST_MODE1_GAP},
        {"mode2.bin", ecmlib::ST_MODE2},
        {"mode2_xa1.bin", ecmlib::ST_MODE2_XA1},
        {"mode2_xa1_gap.bin", ecmlib::ST_MODE2_XA1_GAP}};

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
        appLogger->info("The expected type is {} and the detected type is {}.", (uint8_t)filesToCheck[i].type, (uint8_t)ecmEncoder.get_sector_type());
        if (ecmEncoder.get_sector_type() == filesToCheck[i].type)
        {
            appLogger->info("The detected sector type matches.");
        }
        else
        {
            appLogger->error("The detected sector type doesn't matches the expected one ({}).", (uint8_t)filesToCheck[i].type);
            return 1;
        }
    }

    return 0;
}