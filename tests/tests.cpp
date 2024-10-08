#include "ecmlib.encoder.hpp"
#include "ecmlib.decoder.hpp"
#include <spdlog/sinks/stdout_sinks.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <format>

#include <openssl/evp.h>

// create loggers
auto const appLogger = spdlog::stdout_logger_mt("app_logger");

std::string hash_message(const unsigned char *message, size_t message_len, const EVP_MD *hashtype)
{
    appLogger->trace("Hashing a {} bytes message.", message_len);
    EVP_MD_CTX *mdctx;
    std::vector<unsigned char> digest;
    unsigned int digest_len = 0;

    appLogger->trace("Creating the digest object.");
    if ((mdctx = EVP_MD_CTX_new()) == nullptr)
    {
        appLogger->error("There was an error generating the new CTX object.");
        return std::string();
    }

    EVP_MD_CTX_reset(mdctx);

    if (1 != EVP_DigestInit_ex(mdctx, hashtype, nullptr))
    {
        appLogger->error("There was an error initializing the new CTX object.");
        return std::string();
    }

    appLogger->trace("Adding data to the digest object");
    if (1 != EVP_DigestUpdate(mdctx, message, message_len))
    {
        appLogger->error("There was an error updating the data.");
        return std::string();
    }

    digest.resize(EVP_MD_size(hashtype));

    appLogger->trace("Getting the digest from the CTX object");
    if (1 != EVP_DigestFinal_ex(mdctx, digest.data(), &digest_len))
    {
        appLogger->error("There was an error generating the digest data.");
        return std::string();
    }

    // Generate the stringstream to store the hex characters
    appLogger->trace("Generating the output stream data in hex. Length: {}", digest_len);
    std::string hex = "";

    // Output the hex characters
    for (int i = 0; i < digest_len; i++)
    {
        hex += std::format("{:02x}", digest[i]);
    }

    // Free the CTX object
    appLogger->trace("Free the CTX object");
    EVP_MD_CTX_free(mdctx);

    // Return the string
    appLogger->trace("Returning the string");
    return hex;
}

struct testData
{
    std::string file;
    ecmlib::sector_type type;
    std::string hash = "";
    std::vector<ecmlib::optimizations> opts;
    std::vector<std::string> opts_hash;
    uint16_t sector_number = 0;
};

int main(int argc, char *argv[])
{
    using enum ecmlib::optimizations;
    using enum ecmlib::sector_type;
    // Set logs levels
    auto libLogger = spdlog::stdout_logger_mt(ecmlib::encoder::logger_name());
    appLogger->set_level(spdlog::level::trace);
    libLogger->set_level(spdlog::level::trace);

    // Input buffer
    std::vector<char> inBuffer(2352);
    std::vector<char> encodedBuffer(2352);
    std::vector<char> decodedBuffer(2352);

    // Initialize the ecmlib classes
    auto ecmEncoder = ecmlib::encoder();
    auto ecmDecoder = ecmlib::decoder();

    // Checking the mode of the following files
    std::vector<testData> filesToCheck = {
        {"cdda.bin", ST_CDDA, "93539bdd8c257a5db92d42ad0e78da80", {OO_NONE}, {"93539bdd8c257a5db92d42ad0e78da80"}},
        {"cdda_gap.bin", ST_CDDA_GAP, "9e297efc7a522480ef89a4a7f39ce560", {OO_NONE, OO_REMOVE_GAP}, {"9e297efc7a522480ef89a4a7f39ce560", "d41d8cd98f00b204e9800998ecf8427e"}},
        // Mode 1
        {"mode1.bin",
         ST_MODE1,
         "15da44e7f3478dcc5fbd057d764fc952",
         {
             OO_NONE,
             OO_REMOVE_SYNC,
             OO_REMOVE_SYNC | OO_REMOVE_MSF,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_EDC,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_EDC | OO_REMOVE_BLANKS,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_EDC | OO_REMOVE_BLANKS | OO_REMOVE_ECC,
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
         ST_MODE1_GAP,
         "f1763c7f872304e73caf73a881c34988",
         {
             OO_NONE,
             OO_REMOVE_SYNC,
             OO_REMOVE_SYNC | OO_REMOVE_MSF,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_GAP,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_GAP | OO_REMOVE_EDC,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_GAP | OO_REMOVE_EDC | OO_REMOVE_BLANKS,
             OO_REMOVE_SYNC | OO_REMOVE_MSF | OO_REMOVE_MODE | OO_REMOVE_GAP | OO_REMOVE_EDC | OO_REMOVE_BLANKS | OO_REMOVE_ECC,
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
         ST_MODE1_RAW,
         "e5001282027e56a8feb30c9b2c5bf3ee",
         {OO_NONE},
         {"e5001282027e56a8feb30c9b2c5bf3ee"},
         178},
        // Mode 2
        {"mode2.bin",
         ST_MODE2,
         "76457f1d3c5d3b76fbe16d5ea48d5ca7",
         {OO_NONE},
         {"76457f1d3c5d3b76fbe16d5ea48d5ca7"},
         182},
        // Mode 2 GAP
        {"mode2_gap.bin",
         ST_MODE2_GAP,
         "4fcd456942777be925675cdee81c7cda",
         {OO_NONE},
         {"4fcd456942777be925675cdee81c7cda"},
         759},
        // Mode 2 XA GAP
        {"mode2_xa_gap.bin",
         ST_MODE2_XA_GAP,
         "c5fb890a8853a1027b7741bf2d6a6461",
         {OO_NONE},
         {"c5fb890a8853a1027b7741bf2d6a6461"},

         759},

        // Mode 2 XA1
        {"mode2_xa1.bin",
         ST_MODE2_XA1,
         "6d1b2ccde687e2c19fd77bef1a70a7f2",
         {OO_NONE},
         {"6d1b2ccde687e2c19fd77bef1a70a7f2"},
         759},

        // Mode 2 XA1 GAP
        {"mode2_xa1_gap.bin",
         ST_MODE2_XA1_GAP,
         "d3519e4abafbf30384ecc0a1be63310d",
         {OO_NONE},
         {"d3519e4abafbf30384ecc0a1be63310d"},
         150}};

    for (uint8_t i = 0; i < filesToCheck.size(); i++)
    {
        if (std::ifstream inFile(filesToCheck[i].file); inFile.is_open())
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

        appLogger->debug("Checking the hash of the file.");
        std::string inFileHASH = hash_message((unsigned char *)inBuffer.data(), inBuffer.size(), EVP_md5());
        appLogger->trace("Detected HASH: {} - Original HASH: {}", inFileHASH, filesToCheck[i].hash);
        if (inFileHASH != filesToCheck[i].hash)
        {
            appLogger->error("The input file CRC doesn't matches.");
            return 1;
        }
        else
        {
            appLogger->debug("The input file CRC is correct.");
        }

        ecmEncoder.load(inBuffer.data(), (uint16_t)inBuffer.size());
        ecmlib::sector_type sectorType = ecmEncoder.get_sector_type(inBuffer.data());
        appLogger->info("The expected type is {} and the detected type is {}.", std::to_underlying(filesToCheck[i].type), std::to_underlying(sectorType));
        if (sectorType == filesToCheck[i].type)
        {
            appLogger->info("The detected sector type matches.");
        }
        else
        {
            appLogger->error("The detected sector type doesn't matches the expected one ({}).", std::to_underlying(filesToCheck[i].type));
            return 1;
        }

        for (uint8_t j = 0; j < filesToCheck[i].opts.size(); j++)
        {
            uint16_t encodedSize = 0;
            //  Optimize the sector
            ecmEncoder.encode_sector(inBuffer.data(), (uint16_t)inBuffer.size(), encodedBuffer.data(), (uint16_t)encodedBuffer.size(), encodedSize, filesToCheck[i].opts[j]);

            // Write the output file for debugging
            std::ofstream encFile(std::format("{}.outenc.{}", filesToCheck[i].file, j));
            encFile.write(encodedBuffer.data(), encodedSize);
            encFile.close();

            // Check the CRC
            appLogger->debug("Encoder: Checking the hash of the file with the optimizations {}.", std::to_underlying(filesToCheck[i].opts[j]));
            std::string encodedHASH = hash_message((unsigned char *)encodedBuffer.data(), encodedSize, EVP_md5());
            appLogger->trace("Encoder: Detected HASH: {} - Original HASH: {}", encodedHASH, filesToCheck[i].opts_hash[j]);
            if (encodedHASH != filesToCheck[i].opts_hash[j])
            {
                appLogger->error("The encoded file CRC with the optimizations {} doesn't matches.", std::to_underlying(filesToCheck[i].opts[j]));
                return 1;
            }
            else
            {
                appLogger->debug("The encoded file CRC with the optimizations {} is correct.", std::to_underlying(filesToCheck[i].opts[j]));
            }

            // Decode the data and check if original file can be recovered
            ecmDecoder.decode_sector(encodedBuffer.data(), decodedBuffer.data(), (uint16_t)decodedBuffer.size(), filesToCheck[i].type, filesToCheck[i].sector_number, filesToCheck[i].opts[j]);

            // Write the output file for debugging
            std::ofstream decFile(std::format("{}.outdec.{}", filesToCheck[i].file, j));
            decFile.write(decodedBuffer.data(), decodedBuffer.size());
            decFile.close();

            appLogger->debug("Decoder: Checking the hash of the file with the optimizations {}.", std::to_underlying(filesToCheck[i].opts[j]));
            std::string decodedHASH = hash_message((unsigned char *)decodedBuffer.data(), decodedBuffer.size(), EVP_md5());
            appLogger->trace("Decoder: Detected HASH: {} - Original HASH: {}", decodedHASH, filesToCheck[i].hash);
            if (decodedHASH != filesToCheck[i].hash)
            {
                appLogger->error("The decoded file CRC with the optimizations {} doesn't matches.", std::to_underlying(filesToCheck[i].opts[j]));
                return 1;
            }
            else
            {
                appLogger->debug("The decoded file CRC with the optimizations {} is correct.", std::to_underlying(filesToCheck[i].opts[j]));
            }
        }
    }

    return 0;
}