#include "ecmlib.encoder.hpp"
#include "ecmlib.decoder.hpp"
#include "md5.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <fstream>
#include <iostream>
#include <string>

struct testData
{
    std::string file;
    ecmlib::sector_type type;
    std::string md5 = "";
    std::vector<ecmlib::optimizations> opts;
    std::vector<std::string> opts_md5;
    uint16_t sector_number = 0;
};

int main(int argc, char *argv[])
{
    // create loggers
    auto appLogger = spdlog::stdout_logger_mt("app_logger");
    appLogger->set_level(spdlog::level::trace);
    auto libLogger = spdlog::stdout_logger_mt(ecmlib::encoder::logger_name());
    libLogger->set_level(spdlog::level::trace);

    // Input buffer
    std::vector<char> inBuffer(2352);
    std::vector<char> encodedBuffer(2352);
    std::vector<char> decodedBuffer(2352);

    // Initialize the ecmlib classes
    ecmlib::encoder ecmEncoder = ecmlib::encoder();
    ecmlib::decoder ecmDecoder = ecmlib::decoder();

    MD5 md5;

    // Checking the mode of the following files
    std::vector<testData> filesToCheck = {
        {"cdda.bin", ecmlib::sector_type::ST_CDDA, "93539bdd8c257a5db92d42ad0e78da80", {ecmlib::optimizations::OO_NONE}, {"93539bdd8c257a5db92d42ad0e78da80"}},
        {"cdda_gap.bin", ecmlib::sector_type::ST_CDDA_GAP, "9e297efc7a522480ef89a4a7f39ce560", {ecmlib::optimizations::OO_NONE, ecmlib::optimizations::OO_REMOVE_GAP}, {"9e297efc7a522480ef89a4a7f39ce560", "d41d8cd98f00b204e9800998ecf8427e"}},
        // Mode 1
        {"mode1.bin",
         ecmlib::sector_type::ST_MODE1,
         "15da44e7f3478dcc5fbd057d764fc952",
         {
             ecmlib::optimizations::OO_NONE,
             ecmlib::optimizations::OO_REMOVE_SYNC,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_EDC,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_EDC | ecmlib::optimizations::OO_REMOVE_BLANKS,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_EDC | ecmlib::optimizations::OO_REMOVE_BLANKS | ecmlib::optimizations::OO_REMOVE_ECC,
         },
         {"15da44e7f3478dcc5fbd057d764fc952",
          "715d49c220eebd24cd74e35925b28227",
          "fb05bd6d43d73f8e33ab793a5ee98e3a",
          "d318ea988a8d324d7ec9e129fa63048d",
          "9343e107d47aa51e7f5cf7d938a36f18",
          "ce00edcf27d5794500b70019b97a903a",
          "d72c2cc2a244aa0504db9a45ae459b03"},
         178},

        // Mode 1 GAP
        {"mode1_gap.bin",
         ecmlib::sector_type::ST_MODE1_GAP,
         "f1763c7f872304e73caf73a881c34988",
         {
             ecmlib::optimizations::OO_NONE,
             ecmlib::optimizations::OO_REMOVE_SYNC,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_GAP,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_GAP | ecmlib::optimizations::OO_REMOVE_EDC,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_GAP | ecmlib::optimizations::OO_REMOVE_EDC | ecmlib::optimizations::OO_REMOVE_BLANKS,
             ecmlib::optimizations::OO_REMOVE_SYNC | ecmlib::optimizations::OO_REMOVE_MSF | ecmlib::optimizations::OO_REMOVE_MODE | ecmlib::optimizations::OO_REMOVE_GAP | ecmlib::optimizations::OO_REMOVE_EDC | ecmlib::optimizations::OO_REMOVE_BLANKS | ecmlib::optimizations::OO_REMOVE_ECC,
         },
         {"f1763c7f872304e73caf73a881c34988",
          "dedf21a62c2d2072eaba4279ce0aec22",
          "a65f4a9043fb7094ea3750fb96b8db80",
          "c59a8765d6d223f4cf864ff658acfa02",
          "509bdb286ce0e2ce9f8daf7308375970",

          "a1a39027338ba0abddd08ef81779e888",
          "7caad74b7cf9e03c5ea5de3309f3060d",
          "d41d8cd98f00b204e9800998ecf8427e"},
         150},
        // Mode 1 RAW
        {"mode1_raw.bin",
         ecmlib::sector_type::ST_MODE1_RAW,
         "e5001282027e56a8feb30c9b2c5bf3ee",
         {ecmlib::optimizations::OO_NONE},
         {"e5001282027e56a8feb30c9b2c5bf3ee"},
         178},
        // Mode 2
        {"mode2.bin",
         ecmlib::sector_type::ST_MODE2,
         "76457f1d3c5d3b76fbe16d5ea48d5ca7",
         {ecmlib::optimizations::OO_NONE},
         {"76457f1d3c5d3b76fbe16d5ea48d5ca7"},
         182},
        // Mode 2 GAP
        {"mode2_gap.bin",
         ecmlib::sector_type::ST_MODE2_GAP,
         "4fcd456942777be925675cdee81c7cda",
         {ecmlib::optimizations::OO_NONE},
         {"4fcd456942777be925675cdee81c7cda"},
         759},
        // Mode 2 XA GAP
        {"mode2_xa_gap.bin",
         ecmlib::sector_type::ST_MODE2_XA_GAP,
         "c5fb890a8853a1027b7741bf2d6a6461",
         {ecmlib::optimizations::OO_NONE},
         {"c5fb890a8853a1027b7741bf2d6a6461"},

         759},

        // Mode 2 XA1
        {"mode2_xa1.bin",
         ecmlib::sector_type::ST_MODE2_XA1,
         "6d1b2ccde687e2c19fd77bef1a70a7f2",
         {ecmlib::optimizations::OO_NONE},
         {"6d1b2ccde687e2c19fd77bef1a70a7f2"},
         759},

        // Mode 2 XA1 GAP
        {"mode2_xa1_gap.bin",
         ecmlib::sector_type::ST_MODE2_XA1_GAP,
         "d3519e4abafbf30384ecc0a1be63310d",
         {ecmlib::optimizations::OO_NONE},
         {"d3519e4abafbf30384ecc0a1be63310d"},
         150}};

    for (uint8_t i = 0; i < filesToCheck.size(); i++)
    {
        std::ifstream inFile(filesToCheck[i].file);
        if (inFile.is_open())
        {
            appLogger->info("Reading the file \"{}\".", filesToCheck[i].file);
            inFile.read(inBuffer.data(), inBuffer.size());
            inFile.close();
        }
        else
        {
            appLogger->error("There was an error reading the input file \"{}\".", filesToCheck[i].file);
            return 1;
        }

        appLogger->debug("Checking the md5sum of the file.");
        std::string inFileMD5 = md5(inBuffer.data(), inBuffer.size());
        appLogger->trace("Detected MD5: {} - Original MD5: {}", inFileMD5, filesToCheck[i].md5);
        if (inFileMD5 != filesToCheck[i].md5)
        {
            appLogger->error("The input file CRC doesn't matches.");
            return 1;
        }
        else
        {
            appLogger->debug("The input file CRC is correct.");
        }

        ecmEncoder.load(inBuffer.data(), inBuffer.size());
        ecmlib::sector_type sectorType = ecmEncoder.get_sector_type(inBuffer.data());
        appLogger->info("The expected type is {} and the detected type is {}.", (uint8_t)filesToCheck[i].type, (uint8_t)sectorType);
        if (sectorType == filesToCheck[i].type)
        {
            appLogger->info("The detected sector type matches.");
        }
        else
        {
            appLogger->error("The detected sector type doesn't matches the expected one ({}).", (uint8_t)filesToCheck[i].type);
            return 1;
        }

        for (uint8_t j = 0; j < filesToCheck[i].opts.size(); j++)
        {
            uint16_t encodedSize = 0;
            //  Optimize the sector
            ecmEncoder.encode_sector(inBuffer.data(), inBuffer.size(), encodedBuffer.data(), encodedBuffer.size(), encodedSize, filesToCheck[i].opts[j]);

            // Write the output file for debugging
            std::ofstream encFile(filesToCheck[i].file + ".outenc." + std::to_string(j));
            encFile.write(encodedBuffer.data(), encodedSize);
            encFile.close();

            // Check the CRC
            appLogger->debug("Encoder: Checking the md5sum of the file with the optimizations {}.", (uint8_t)filesToCheck[i].opts[j]);
            std::string encodedMD5 = md5(encodedBuffer.data(), encodedSize);
            appLogger->trace("Encoder: Detected MD5: {} - Original MD5: {}", encodedMD5, filesToCheck[i].opts_md5[j]);
            if (encodedMD5 != filesToCheck[i].opts_md5[j])
            {
                appLogger->error("The encoded file CRC with the optimizations {} doesn't matches.", (uint8_t)filesToCheck[i].opts[j]);
                return 1;
            }
            else
            {
                appLogger->debug("The encoded file CRC with the optimizations {} is correct.", (uint8_t)filesToCheck[i].opts[j]);
            }

            // Decode the data and check if original file can be recovered

            ecmDecoder.decode_sector(encodedBuffer.data(), encodedSize, decodedBuffer.data(), decodedBuffer.size(), filesToCheck[i].type, filesToCheck[i].sector_number, filesToCheck[i].opts[j]);

            // Write the output file for debugging
            std::ofstream decFile(filesToCheck[i].file + ".outdec." + std::to_string(j));
            decFile.write(decodedBuffer.data(), decodedBuffer.size());
            decFile.close();

            appLogger->debug("Decoder: Checking the md5sum of the file with the optimizations {}.", (uint8_t)filesToCheck[i].opts[j]);
            std::string decodedMD5 = md5(decodedBuffer.data(), decodedBuffer.size());
            appLogger->trace("Decoder: Detected MD5: {} - Original MD5: {}", decodedMD5, filesToCheck[i].md5);
            if (decodedMD5 != filesToCheck[i].md5)
            {
                appLogger->error("The decoded file CRC with the optimizations {} doesn't matches.", (uint8_t)filesToCheck[i].opts[j]);
                return 1;
            }
            else
            {
                appLogger->debug("The decoded file CRC with the optimizations {} is correct.", (uint8_t)filesToCheck[i].opts[j]);
            }
        }
    }

    return 0;
}