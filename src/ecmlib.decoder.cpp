#include "ecmlib.decoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    decoder::decoder() : base()
    {
        // Initialize the logger
        mLogger = spdlog::get(ecmLoggerName);
        // Check if the logger exists or segfault will happen when used
        if (mLogger == nullptr)
        {
            // Doesn't exists, so a new logger with level off will be created.
            auto libLogger = spdlog::stdout_logger_mt(ecmLoggerName);
            libLogger->set_level(spdlog::level::off);
            // Return the new created logger
            mLogger = spdlog::get(ecmLoggerName);
        }

        mLogger->debug("Initializing decoder class.");

        mLogger->debug("Finished the decoder class inizialization.");
    }

    status_code decoder::decode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer,
                                       uint16_t outBufferSize, sector_type sectorType, uint16_t sectorNumber, optimizations opts)
    {
        // Check if the output buffer can store the full sector
        if (outBufferSize < 2352)
        {
            // It can't
            mLogger->error("The output buffer is smaller than the output data.");
            return status_code::STATUS_ERROR_NO_ENOUGH_BUFFER_SPACE;
        }

        // Keep the current readed position
        uint16_t currentPos = 0;

        //
        //
        // SYNC Data
        //
        //
        if (sectorType != sector_type::ST_CDDA && sectorType != sector_type::ST_CDDA_GAP)
        {
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_SYNC))
            {
                mLogger->trace("Copying the SYNC Data from position {}.", currentPos);
                std::memcpy(outBuffer, inBuffer + currentPos, 0xC);
                currentPos += 0xC;
                mLogger->trace("SYNC data copied.");
            }
            else
            {
                // Write the sync data
                mLogger->trace("Generating the SYNC Data.");
                outBuffer[0x0] = 0x00;
                outBuffer[0x1] = 0xFF;
                outBuffer[0x2] = 0xFF;
                outBuffer[0x3] = 0xFF;
                outBuffer[0x4] = 0xFF;
                outBuffer[0x5] = 0xFF;
                outBuffer[0x6] = 0xFF;
                outBuffer[0x7] = 0xFF;
                outBuffer[0x8] = 0xFF;
                outBuffer[0x9] = 0xFF;
                outBuffer[0xA] = 0xFF;
                outBuffer[0xB] = 0x00;
                mLogger->trace("SYNC Data generated.");
            }
        }

        //
        //
        // MSF Data
        //
        //
        if (sectorType != sector_type::ST_CDDA && sectorType != sector_type::ST_CDDA_GAP)
        {
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MSF))
            {
                // All but the RAW CDDA sector will have MSF data. If the optimization to remove it is not set, then copy it.
                mLogger->trace("Copying the MSF Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0xC, inBuffer + currentPos, 0x4);
                currentPos += 3;
                mLogger->trace("MSF Data copied.");
            }
            else
            {
                mLogger->trace("Generating the MSF Data with the sector number {}", sectorNumber);
                std::vector<char> msf = sector_to_time(sectorNumber);
                outBuffer[0xC] = msf[0];
                outBuffer[0xD] = msf[1];
                outBuffer[0xE] = msf[2];
                mLogger->trace("MSF Data generated.");
            }
        }

        //
        //
        // Mode Data
        //
        //
        if (sectorType != sector_type::ST_CDDA && sectorType != sector_type::ST_CDDA_GAP)
        {
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MODE))
            {
                // All but the RAW CDDA sector will have MODE data. If the optimization to remove it is not set, then copy it.
                mLogger->trace("Copying the MODE Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0xF, inBuffer + currentPos, 1);
                currentPos += 1;
                mLogger->trace("MODE Data copied.");
            }
            else
            {
                mLogger->trace("Generating MODE Data.");
                if (
                    sectorType == sector_type::ST_MODE1 ||
                    sectorType == sector_type::ST_MODE1_GAP ||
                    sectorType == sector_type::ST_MODE1_RAW)
                {
                    outBuffer[0xF] = 0x1;
                }
                else
                {
                    outBuffer[0xF] = 0x2;
                }
                mLogger->trace("MODE Data generated.");
            }
        }

        //
        //
        // Flags Data.
        //
        //
        if (sectorType == sector_type::ST_MODE2_XA_GAP ||
            sectorType == sector_type::ST_MODE2_XA1 ||
            sectorType == sector_type::ST_MODE2_XA1_GAP ||
            sectorType == sector_type::ST_MODE2_XA2 ||
            sectorType == sector_type::ST_MODE2_XA2_GAP)
        {
            // Only Mode 2 XA will have FLAGS
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_REDUNDANT_FLAG))
            {
                // If the optimization to remove it is not set, then fully copy both.
                mLogger->trace("Copying the two copies of the FLAGS Data from position {}", currentPos);
                std::memcpy(outBuffer + 0X10, inBuffer + currentPos, 0x8);
                currentPos += 8;
                mLogger->trace("FLAGS Data copied.");
            }
            else
            {
                // If the optimization to remove it is enabled, then copy it twice.
                mLogger->trace("Copying twice the unique copy of the FLAGS Data from position {}", currentPos);
                std::memcpy(outBuffer + 0X10, inBuffer + currentPos, 0x4);
                std::memcpy(outBuffer + 0X14, inBuffer + currentPos, 0x4);
                currentPos += 4;
                mLogger->trace("FLAGS Data copied (twice).");
            }
        }

        //
        //
        // Sector Data. Everybody has data...
        //
        //
        switch (sectorType)
        {
        case sector_type::ST_CDDA:
        case sector_type::ST_CDDA_GAP:
            // CDDA sectors are fully raw, so all will be copied if it's not a GAP with the GAP optimization enabled.
            if (sectorType == sector_type::ST_CDDA || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP))
            {
                mLogger->trace("Copying the CDDA Data from position {}.", currentPos);
                std::memcpy(outBuffer, inBuffer, 0x930);
                currentPos += 0x930;
                mLogger->trace("CDDA data copied.");
            }
            else
            {
                // We must generate the GAP sector
                mLogger->trace("Generating the CDDA GAP data.");
                std::memset(outBuffer, 0x0, 0x930);
                mLogger->trace("CDDA GAP data generated.");
            }
            // Note: encodedDataSize should be 0
            break;

        case sector_type::ST_MODE1:
        case sector_type::ST_MODE1_RAW:
        case sector_type::ST_MODE1_GAP:
            // Mode1 sectors starts at 0x10 and ends at 0x80F
            if (sectorType == sector_type::ST_MODE1 || sectorType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP))
            {
                mLogger->trace("Copying the MODE1 Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x10, inBuffer + currentPos, 0x800);
                currentPos += 0x800;
                mLogger->trace("MODE1 Data copied.");
            }
            else
            {
                // We must generate the GAP sector
                mLogger->trace("Generating the MODE1 GAP Data.");
                std::memset(outBuffer + 0x10, 0x0, 0x800);
                mLogger->trace("MODE1 GAP Data generated.");
            }
            break;

        case sector_type::ST_MODE2:
        case sector_type::ST_MODE2_GAP:
            // Mode2 sectors starts at 0x10 and ends at 0x92F
            if (sectorType == sector_type::ST_MODE2 || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP))
            {
                mLogger->trace("Copying the MODE2 Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x10, inBuffer + currentPos, 0x920);
                currentPos += 0x920;
                mLogger->trace("MODE2 Data copied.");
            }
            else
            {
                // We must generate the GAP sector
                mLogger->trace("Generating the MODE2 GAP Data.");
                std::memset(outBuffer + 0x10, 0x0, 0x920);
                mLogger->trace("MODE2 GAP Data generated.");
            }
            break;

        case sector_type::ST_MODE2_XA1:
        case sector_type::ST_MODE2_XA1_GAP:
        // The unknown XA mode (GAP) will be threated as XA1 because is mainly used in PSX games
        case sector_type::ST_MODE2_XA_GAP:
            // Mode1 sectors starts at 0x18 and ends at 0x817
            if (sectorType == sector_type::ST_MODE2_XA1 || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP))
            {
                mLogger->trace("Copying the MODE2 XA1 Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x18, inBuffer + currentPos, 0x800);
                currentPos += 0x800;
                mLogger->trace("MODE2 XA1 Data copied.");
            }
            else
            {
                // We must generate the GAP sector
                mLogger->trace("Generating the MODE2 XA1 GAP Data.");
                std::memset(outBuffer + 0x18, 0x0, 0x800);
                mLogger->trace("MODE2 XA1 GAP Data generated.");
            }
            break;

        case sector_type::ST_MODE2_XA2:
        case sector_type::ST_MODE2_XA2_GAP:
            // Mode2 XA2 sectors starts at 0x18 and ends at 0x92B
            if (sectorType == sector_type::ST_MODE2_XA2 || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP))
            {
                mLogger->trace("Copying the MODE2 XA2 Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x18, inBuffer + currentPos, 0x914);
                currentPos += 0x914;
                mLogger->trace("MODE2 XA2 Data copied.");
            }
            else
            {
                // We must generate the GAP sector
                mLogger->trace("Generating the MODE2 XA2 GAP Data.");
                std::memset(outBuffer + 0x18, 0x0, 0x914);
                mLogger->trace("MODE2 XA2 GAP Data generated.");
            }
            break;

        default:
            break;
        }

        //
        //
        // EDC data. Mode 1 and Mode 2 XA.
        //
        //
        switch (sectorType)
        {
        case sector_type::ST_MODE1:
        case sector_type::ST_MODE1_RAW:
        case sector_type::ST_MODE1_GAP:
            // Mode1 EDC starts at 0x810 and ends at 0x813
            if (sectorType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC))
            {
                mLogger->trace("Copying the MODE1 EDC Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x810, inBuffer + currentPos, 0x4);
                currentPos += 4;
                mLogger->trace("EDC data copied.");
            }
            else
            {
                // Generate the EDC and write it to output
                mLogger->trace("Generating the MODE1 EDC data.");
                uint32_t edc = edc_compute(outBuffer, 0x810);
                put32lsb(outBuffer + 0x810, edc);
                mLogger->trace("EDC Data written.");
            }

            break;

        case sector_type::ST_MODE2_XA1:
        case sector_type::ST_MODE2_XA1_GAP:
        case sector_type::ST_MODE2_XA_GAP:
            // Mode2 XA1 EDC starts at 0x818 and ends at 0x81B
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC))
            {
                mLogger->trace("Copying the MODE2 XA1 EDC Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x818, inBuffer + currentPos, 0x4);
                currentPos += 4;
                mLogger->trace("EDC data copied.");
            }
            else
            {
                // Generate the EDC and write it to output
                mLogger->trace("Generating the MODE2 XA1 EDC data");
                uint32_t edc = edc_compute(outBuffer + 0x10, 0x808);
                put32lsb(outBuffer + 0x818, edc);
                mLogger->trace("EDC Data written");
            }

            break;

        case sector_type::ST_MODE2_XA2:
        case sector_type::ST_MODE2_XA2_GAP:
            // Mode2 XA2 EDC starts at 0x92C and ends at 0x92F
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC))
            {
                mLogger->trace("Copying the MODE2 XA2 EDC Data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x92C, inBuffer + currentPos, 0x4);
                currentPos += 4;
                mLogger->trace("EDC data copied.");
            }
            else
            {
                // Generate the EDC and write it to output
                mLogger->trace("Generating the MODE2 XA2 EDC data");
                uint32_t edc = edc_compute(outBuffer + 0x10, 0x91C);
                put32lsb(outBuffer + 0x92C, edc);
                mLogger->trace("EDC Data written");
            }

            break;

        default:
            break;
        }

        //
        //
        // Blank data. Mode 1
        //
        //
        if (sectorType == sector_type::ST_MODE1 ||
            sectorType == sector_type::ST_MODE1_GAP ||
            sectorType == sector_type::ST_MODE1_RAW)
        {
            if (sectorType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_BLANKS))
            {
                // Mode1 Blank data starts at 0x814 and ends at 0x81B
                mLogger->trace("Copying the input blank data from position {}.", currentPos);
                std::memcpy(outBuffer + 0x814, inBuffer + currentPos, 0x8);
                currentPos += 0x8;
                mLogger->trace("Blank data copied.");
            }
            else
            {
                // We must generate the GAP data
                mLogger->trace("Generating the blank data.");
                std::memset(outBuffer + 0x814, 0x0, 0x8);
                mLogger->trace("Blank data generated.");
            }
        }

        //
        //
        // Mode 1 and Mode 2 XA1 Correction Code
        //
        //
        if (sectorType == sector_type::ST_MODE1 ||
            sectorType == sector_type::ST_MODE1_RAW ||
            sectorType == sector_type::ST_MODE1_GAP ||
            sectorType == sector_type::ST_MODE2_XA1 ||
            sectorType == sector_type::ST_MODE2_XA1_GAP ||
            sectorType == sector_type::ST_MODE2_XA_GAP)
        {
            if (sectorType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(opts & optimizations::OO_REMOVE_ECC))
            {
                mLogger->trace("Copying the input correction code from position {}.", currentPos);
                std::memcpy(outBuffer + 0x81C, inBuffer + currentPos, 0x114);
                currentPos += 0x114;
                mLogger->trace("Input correction code copied.");
            }
            else
            {
                // Generating ECC Data
                mLogger->trace("Generating the correction code.");
                switch (sectorType)
                {
                case sector_type::ST_MODE1:
                case sector_type::ST_MODE1_GAP:
                    ecc_write_sector(
                        reinterpret_cast<uint8_t *>(outBuffer + 0xC),
                        reinterpret_cast<uint8_t *>(outBuffer + 0x10),
                        reinterpret_cast<uint8_t *>(outBuffer + 0x81C));
                    break;

                case sector_type::ST_MODE2_XA1:
                case sector_type::ST_MODE2_XA1_GAP:
                case sector_type::ST_MODE2_XA_GAP:
                    ecc_write_sector(
                        zeroaddress.data(),
                        reinterpret_cast<uint8_t *>(outBuffer + 0x10),
                        reinterpret_cast<uint8_t *>(outBuffer + 0x81C));
                    break;

                default:
                    break;
                }
                mLogger->trace("Error correction code generated.");
            }
        }

        return status_code::STATUS_OK;
    }

    std::vector<char> inline decoder::sector_to_time(uint32_t sectorNumber)
    {
        std::vector<char> timeData(3);
        uint8_t sectors = sectorNumber % 75;
        uint8_t seconds = (sectorNumber / 75) % 60;
        uint8_t minutes = (sectorNumber / 75) / 60;

        // Converting decimal to hex base 10
        // 15 -> 0x15 instead 0x0F
        timeData[0] = (minutes / 10 * 16) + (minutes % 10);
        timeData[1] = (seconds / 10 * 16) + (seconds % 10);
        timeData[2] = (sectors / 10 * 16) + (sectors % 10);
        return timeData;
    }
}