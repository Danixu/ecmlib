#include "ecmlib.decoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    decoder::decoder() : base()
    {
        mLogger->debug("Initializing decoder class.");

        mLogger->debug("Finished the decoder class inizialization.");
    }

    /**
     * @brief Object cleanup
     *
     */
    decoder::~decoder()
    {
    }

    status_code decoder::decode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer,
                                       uint16_t outBufferSize, sector_type sectorType, uint16_t sectorNumber, optimizations opts)
    {
        // Keep the current readed position
        uint16_t currentPos = 0;

        //
        //
        // SYNC Data
        //
        //
        if (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP)
        {
            if (opts & OO_REMOVE_SYNC)
            {
                // Write the sync data
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
            }
            else
            {
                std::memcpy(outBuffer, inBuffer + currentPos, 0xC);
                currentPos += 0xC;
            }
        }

        //
        //
        // MSF Data
        //
        //
        if (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP)
        {
            if (opts & OO_REMOVE_MSF)
            {
                std::vector<char> msf = sector_to_time(sectorNumber);
                outBuffer[0xC] = msf[0];
                outBuffer[0xD] = msf[1];
                outBuffer[0xE] = msf[2];
            }
            else
            {
                // All but the RAW CDDA sector will have MSF data. If the optimization to remove it is not set, then copy it.
                std::memcpy(outBuffer + 0xC, inBuffer + currentPos, 0x4);
                currentPos += 3;
            }
        }

        //
        //
        // Mode Data
        //
        //
        if (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP)
        {
            if (!(opts & OO_REMOVE_MODE))
            {
                // All but the RAW CDDA sector will have MODE data. If the optimization to remove it is not set, then copy it.
                std::memcpy(outBuffer + 0xF, inBuffer + currentPos, 1);
                currentPos += 1;
            }
            else
            {
                if (
                    sectorType == ST_MODE1 ||
                    sectorType == ST_MODE1_GAP ||
                    sectorType == ST_MODE1_RAW)
                {
                    outBuffer[0xF] = 0x1;
                }
                else
                {
                    outBuffer[0xF] = 0x2;
                }
            }
        }

        //
        //
        // Sector Data. Everybody has data...
        //
        //
        switch (sectorType)
        {
        case ST_CDDA:
        case ST_CDDA_GAP:
            // CDDA sectors are fully raw, so all will be copied if it's not a GAP with the GAP optimization enabled.
            if (sectorType == ST_CDDA || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer, inBuffer, 0x930);
                currentPos += 0x930;
            }
            else
            {
                // We must generate the GAP sector
                std::memset(outBuffer, 0x0, 0x930);
                currentPos += 0x930;
            }
            // Note: encodedDataSize should be 0
            break;

        case ST_MODE1:
        case ST_MODE1_RAW:
        case ST_MODE1_GAP:
            // Mode1 sectors starts at 0x10 and ends at 0x80F
            if (sectorType == ST_MODE1 || sectorType == ST_MODE1_RAW || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + 0x10, inBuffer + currentPos, 0x800);
                currentPos += 0x800;
            }
            else
            {
                // We must generate the GAP sector
                std::memset(outBuffer + 0x10, 0x0, 0x800);
                currentPos += 0x800;
            }
            break;

        case ST_MODE2:
        case ST_MODE2_GAP:
            // Mode2 sectors starts at 0x10 and ends at 0x92F
            if (sectorType == ST_MODE2 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + 0x10, inBuffer + currentPos, 0x920);
                currentPos += 0x920;
            }
            else
            {
                // We must generate the GAP sector
                std::memset(outBuffer + 0x10, 0x0, 0x920);
                currentPos += 0x920;
            }
            break;

        case ST_MODE2_XA1:
        case ST_MODE2_XA1_GAP:
        // The unknown XA mode (GAP) will be threated as XA1 because is mainly used in PSX games
        case ST_MODE2_XA_GAP:
            // Mode1 sectors starts at 0x18 and ends at 0x817
            if (sectorType == ST_MODE2_XA1 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + 0x18, inBuffer + currentPos, 0x800);
                currentPos += 0x800;
            }
            else
            {
                // We must generate the GAP sector
                std::memset(outBuffer + 0x18, 0x0, 0x800);
                currentPos += 0x800;
            }
            break;

        case ST_MODE2_XA2:
        case ST_MODE2_XA2_GAP:
            // Mode2 XA2 sectors starts at 0x18 and ends at 0x92B
            if (sectorType == ST_MODE2_XA2 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + 0x18, inBuffer + currentPos, 0x914);
                currentPos += 0x914;
            }
            else
            {
                // We must generate the GAP sector
                std::memset(outBuffer + 0x18, 0x0, 0x914);
                currentPos += 0x914;
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
        case ST_MODE1:
        case ST_MODE1_RAW:
        case ST_MODE1_GAP:
            // Mode1 EDC starts at 0x810 and ends at 0x813
            if (sectorType == ST_MODE1_RAW || !(opts & OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + 0x810, inBuffer + currentPos, 0x4);
                currentPos += 4;
            }
            else
            {
                // Generate the EDC and write it to output
                uint32_t edc = edc_compute(outBuffer, 0x810);
                put32lsb(outBuffer + 0x810, edc);
                currentPos += 4;
            }

            break;

        case ST_MODE2_XA1:
        case ST_MODE2_XA1_GAP:
        case ST_MODE2_XA_GAP:
            // Mode2 XA1 EDC starts at 0x818 and ends at 0x81B
            if (!(opts & OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + 0x818, inBuffer + currentPos, 0x4);
                currentPos += 4;
            }
            else
            {
                // Generate the EDC and write it to output
                uint32_t edc = edc_compute(outBuffer + 0x10, 0x808);
                put32lsb(outBuffer + 0x818, edc);
                currentPos += 4;
            }

            break;

        case ST_MODE2_XA2:
        case ST_MODE2_XA2_GAP:
            // Mode2 XA2 EDC starts at 0x92C and ends at 0x92F
            if (!(opts & OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + 0x92C, inBuffer + currentPos, 0x4);
                currentPos += 4;
            }
            else
            {
                // Generate the EDC and write it to output
                uint32_t edc = edc_compute(outBuffer + 0x10, 0x91C);
                put32lsb(outBuffer + 0x92C, edc);
                currentPos += 4;
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
        if ((sectorType == ST_MODE1 ||
             sectorType == ST_MODE1_GAP ||
             sectorType == ST_MODE1_RAW) &&
            (sectorType == ST_MODE1_RAW || !(opts & OO_REMOVE_BLANKS)))
        {
            // Mode1 Blank data starts at 0x814 and ends at 0x81B
            std::memcpy(outBuffer + 0x814, inBuffer + currentPos, 0x8);
            currentPos += 8;
        }
        else
        {
            // We must generate the GAP sector
            std::memset(outBuffer + currentPos, 0x0, 0x8);
            currentPos += 0x8;
        }

        //
        //
        // Mode 1 and Mode 2 XA1 Correction Code
        //
        //
        if ((sectorType == ST_MODE1 ||
             sectorType == ST_MODE1_RAW ||
             sectorType == ST_MODE1_GAP ||
             sectorType == ST_MODE2_XA1 ||
             sectorType == ST_MODE2_XA1_GAP ||
             sectorType == ST_MODE2_XA_GAP) &&
            (sectorType == ST_MODE1_RAW || !(opts & OO_REMOVE_ECC)))
        {
            std::memcpy(outBuffer + 0x81C, inBuffer + currentPos, 0x114);
            currentPos += 0x114;
        }
        else
        {
            // Generating ECC Data
            switch (sectorType)
            {
            case ST_MODE1:
            case ST_MODE1_GAP:
                ecc_write_sector(
                    reinterpret_cast<uint8_t *>(outBuffer + 0xC),
                    reinterpret_cast<uint8_t *>(outBuffer + 0x10),
                    reinterpret_cast<uint8_t *>(outBuffer + currentPos));
                break;

            case ST_MODE2_XA1:
            case ST_MODE2_XA1_GAP:
                ecc_write_sector(
                    zeroaddress,
                    reinterpret_cast<uint8_t *>(outBuffer + 0x10),
                    reinterpret_cast<uint8_t *>(outBuffer + currentPos));
                break;

            case ST_MODE2_XA_GAP:
                ecc_write_sector(
                    zeroaddress,
                    reinterpret_cast<uint8_t *>(outBuffer + 0x10),
                    reinterpret_cast<uint8_t *>(outBuffer + currentPos));
                break;

            default:
                break;
            }
        }

        return STATUS_OK;
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