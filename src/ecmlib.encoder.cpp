#include "ecmlib.encoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    encoder::encoder() : base()
    {
        mLogger->debug("Initializing encoder class.");

        mLogger->debug("Finished the encoder class inizialization.");
    }

    /**
     * @brief Object cleanup
     *
     */
    encoder::~encoder()
    {
    }

    /**
     * @brief Optimize the sector and store the data into the internal buffer
     *
     * @param force Forces the optimization even if to recover the original sector is not possible
     * @param onlyData Remove all except the data from the sector
     * @return status_code Status code
     */
    status_code encoder::encode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer, uint16_t outBufferSize, uint16_t &encodedDataSize, optimizations opts)
    {
        mLogger->debug("Encoding the sector...");
        // Check if the sector was loaded
        if (inBufferSize == 0)
        {
            mLogger->error("There is no input data. Execute the load method to load it first.");
            return STATUS_ERROR_NO_DATA;
        }

        // Detect the sector type
        sector_type sectorType = get_sector_type(inBuffer);

        // The sector is Unknown, so there were no enough data to detect it.
        if (sectorType == ST_UNKNOWN)
        {
            mLogger->error("Sector type is unknown. Maybe there is no enough data to determine the type.");
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        encodedDataSize = 0;
        //
        //
        // SYNC Data
        //
        //
        if (!(opts & OO_REMOVE_SYNC) &&
            (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP))
        {
            // All but the RAW CDDA sector will have sync data. If the optimization to remove it is not set, then copy it.
            std::memcpy(outBuffer + encodedDataSize, inBuffer, 0xC);
            encodedDataSize += 12;
        }

        //
        //
        // MSF Data
        //
        //
        if (!(opts & OO_REMOVE_MSF) &&
            (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP))
        {
            // All but the RAW CDDA sector will have MSF data. If the optimization to remove it is not set, then copy it.
            std::memcpy(outBuffer + encodedDataSize, inBuffer + 0xC, 0x4);
            encodedDataSize += 3;
        }

        //
        //
        // Mode Data
        //
        //
        if (!(opts & OO_REMOVE_MODE) &&
            (sectorType != ST_CDDA && sectorType != ST_CDDA_GAP))
        {
            // All but the RAW CDDA sector will have MODE data. If the optimization to remove it is not set, then copy it.
            std::memcpy(outBuffer + encodedDataSize, inBuffer + 0xF, 1);
            encodedDataSize += 1;
        }

        //
        //
        // Flags Data.
        //
        //
        if (sectorType == ST_MODE2_XA_GAP ||
            sectorType == ST_MODE2_XA1 ||
            sectorType == ST_MODE2_XA1_GAP ||
            sectorType == ST_MODE2_XA2 ||
            sectorType == ST_MODE2_XA2_GAP)
        {
            // Only Mode 2 XA will have FLAGS
            if (!(opts & OO_REMOVE_REDUNDANT_FLAG))
            {
                // If the optimization to remove it is not set, then fully copy both.
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0X10, 0x8);
                encodedDataSize += 8;
            }
            else
            {
                // If the optimization to remove it is enabled, then copy only one of them.
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0X10, 0x4);
                encodedDataSize += 4;
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
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // CDDA sectors are fully raw, so all will be copied if it's not a GAP with the GAP optimization enabled.
            if (sectorType == ST_CDDA || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer, 0x930);
                encodedDataSize += 0x930;
                _sectorDataLink.dataSize = 0x930;
            }
            else
            {
                _sectorDataLink.dataSize = 0;
            }
            // Note: encodedDataSize should be 0
            break;

        case ST_MODE1:
        case ST_MODE1_RAW:
        case ST_MODE1_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode1 sectors starts at 0x10 and ends at 0x80F
            if (sectorType == ST_MODE1 || sectorType == ST_MODE1_RAW || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x10, 0x800);
                encodedDataSize += 0x800;
                _sectorDataLink.dataSize = 0x800;
            }
            else
            {
                _sectorDataLink.dataSize = 0;
            }
            break;

        case ST_MODE2:
        case ST_MODE2_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode2 sectors starts at 0x10 and ends at 0x92F
            if (sectorType == ST_MODE2 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x10, 0x920);
                encodedDataSize += 0x920;
                _sectorDataLink.dataSize = 0x920;
            }
            else
            {
                _sectorDataLink.dataSize = 0;
            }
            break;

        case ST_MODE2_XA1:
        case ST_MODE2_XA1_GAP:
        // The unknown XA mode (GAP) will be threated as XA1 because is mainly used in PSX games
        case ST_MODE2_XA_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode1 sectors starts at 0x18 and ends at 0x817
            if (sectorType == ST_MODE2_XA1 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x18, 0x800);
                encodedDataSize += 0x800;
                _sectorDataLink.dataSize = 0x800;
            }
            else
            {
                _sectorDataLink.dataSize = 0;
            }
            break;

        case ST_MODE2_XA2:
        case ST_MODE2_XA2_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode2 XA2 sectors starts at 0x18 and ends at 0x92B
            if (sectorType == ST_MODE2_XA2 || !(opts & OO_REMOVE_GAP))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x18, 0x914);
                encodedDataSize += 0x914;
                _sectorDataLink.dataSize = 0x914;
            }
            else
            {
                _sectorDataLink.dataSize = 0;
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
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x810, 0x4);
                encodedDataSize += 4;
            }

            break;

        case ST_MODE2_XA1:
        case ST_MODE2_XA1_GAP:
        case ST_MODE2_XA_GAP:
            // Mode2 XA1 EDC starts at 0x818 and ends at 0x81B
            if (!(opts & OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x818, 0x4);
                encodedDataSize += 4;
            }

            break;

        case ST_MODE2_XA2:
        case ST_MODE2_XA2_GAP:
            // Mode2 XA2 EDC starts at 0x92C and ends at 0x92F
            if (!(opts & OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x92C, 0x4);
                encodedDataSize += 4;
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
            std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x814, 0x8);
            encodedDataSize += 8;
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
            std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x81C, 0x114);
            encodedDataSize += 0x114;
        }

        mLogger->debug("Optimization finished.");
        return STATUS_OK;
    }

    /**
     * @brief Detects if the sector is a gap or contains data
     *
     * @param sector (uint8_t*) Stream containing all the bytes which will be compared
     * @param length (uint16_t) Length of the stream to check
     * @return true The sectors are a gap
     * @return false Any or all sectors are not zeroed, so is not a gap.
     */
    bool inline encoder::is_gap(char *sector, size_t length)
    {
        mLogger->trace("Checking {} bytes for a gap.", length);
        for (size_t i = 0; i < length; i++)
        {
            if ((sector[i]) != 0x00)
            {
                mLogger->trace("Received data is not a GAP. Detected non gap at {}.", i);
                return false; // Sector contains data, so is not a GAP
            }
        }

        mLogger->trace("Received data is a GAP.");
        return true;
    }

    sector_type encoder::get_sector_type(char *inputSector)
    {
        mLogger->debug("Detecting the sector type.");
        if (
            inputSector[0x000] == (char)0x00 && // sync (12 bytes)
            inputSector[0x001] == (char)0xFF &&
            inputSector[0x002] == (char)0xFF &&
            inputSector[0x003] == (char)0xFF &&
            inputSector[0x004] == (char)0xFF &&
            inputSector[0x005] == (char)0xFF &&
            inputSector[0x006] == (char)0xFF &&
            inputSector[0x007] == (char)0xFF &&
            inputSector[0x008] == (char)0xFF &&
            inputSector[0x009] == (char)0xFF &&
            inputSector[0x00A] == (char)0xFF &&
            inputSector[0x00B] == (char)0x00)
        {
            mLogger->trace("Sync data detected... Sector is a data sector.");
            // Sector is a MODE1/MODE2 sector
            if (
                inputSector[0x00F] == (char)0x01 && // mode (1 byte)
                inputSector[0x814] == (char)0x00 && // reserved (8 bytes)
                inputSector[0x815] == (char)0x00 &&
                inputSector[0x816] == (char)0x00 &&
                inputSector[0x817] == (char)0x00 &&
                inputSector[0x818] == (char)0x00 &&
                inputSector[0x819] == (char)0x00 &&
                inputSector[0x81A] == (char)0x00 &&
                inputSector[0x81B] == (char)0x00)
            {
                mLogger->trace("Sector is a MODE1 sector. Checking EDC...");
                //  The sector is surelly MODE1 but we will check the EDC
                if (
                    ecc_check_sector(
                        reinterpret_cast<uint8_t *>(inputSector + 0xC),
                        reinterpret_cast<uint8_t *>(inputSector + 0x10),
                        reinterpret_cast<uint8_t *>(inputSector + 0x81C)) &&
                    edc_compute(inputSector, 0x810) == get32lsb(inputSector + 0x810))
                {
                    mLogger->trace("Mode 1 sector detected. Determining if it's a GAP.");
                    if (is_gap(inputSector + 0x010, 0x800))
                    {
                        mLogger->debug("The sector is at Mode 1 GAP.");
                        return ST_MODE1_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 1.");
                        return ST_MODE1; // Mode 1
                    }
                }

                // If EDC doesn't match, then the sector is damaged. It can be a protection method, so will be threated as RAW.
                mLogger->trace("The EDC cannot be verified, so the sector will be threated as RAW.");
                return ST_MODE1_RAW;
            }
            else if (
                inputSector[0x00F] == (char)0x02 // mode 2 (1 byte)
            )
            {
                //  The sector is MODE2, and now we will detect what kind
                //
                // Might be Mode 2 GAP.
                //
                mLogger->trace("Mode 2 sector detected. Checking if is just a GAP");
                if (is_gap(inputSector + 0x10, 0x920))
                {
                    mLogger->trace("Mode 2 GAP detected.");
                    return ST_MODE2_GAP;
                }
                //
                // Might be Mode 2 XA GAP
                //
                mLogger->trace("Checking if is an XA GAP sector type. "
                               "Wrong, but used in some games and can free some space.");
                if ((inputSector[0x10] == inputSector[0x14] && // First we must check if contains the XA FLAG
                     inputSector[0x11] == inputSector[0x15] &&
                     inputSector[0x12] == inputSector[0x16] &&
                     inputSector[0x13] == inputSector[0x17]) &&
                    is_gap(inputSector + 0x18, 0x918)) // Then check if it's a GAP
                {
                    mLogger->trace("Mode 2 XA GAP detected.");
                    return ST_MODE2_XA_GAP;
                }
                //
                // Might be Mode 2, XA 1 or 2
                //
                mLogger->trace("Mode 2 sector detected. Determining if XA 1 or XA 2.");
                if (ecc_check_sector(zeroaddress,
                                     reinterpret_cast<uint8_t *>(inputSector + 0x10),
                                     reinterpret_cast<uint8_t *>(inputSector + 0x81C)) &&
                    edc_compute(inputSector + 0x10, 0x808) == get32lsb(inputSector + 0x818))
                {
                    mLogger->trace("Mode 2 XA 1 detected. Checking if it's a GAP.");
                    if (is_gap(inputSector + 0x018, 0x800))
                    {
                        mLogger->debug("The sector is at Mode 2 XA 1 GAP.");
                        return ST_MODE2_XA1_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 2 XA 1.");
                        return ST_MODE2_XA1; //  Mode 2, XA 1
                    }
                }
                //
                // Might be Mode 2, XA 2
                //
                if (
                    edc_compute(inputSector + 0x10, 0x91C) == get32lsb(inputSector + 0x92C))
                {
                    mLogger->trace("Mode 2 XA 2 detected. Checking if it's a GAP.");
                    if (is_gap(inputSector + 0x018, 0x914))
                    {
                        mLogger->debug("The sector is at Mode 2 XA 2 GAP.");
                        return ST_MODE2_XA2_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 2 XA 2.");
                        return ST_MODE2_XA2; // Mode 2, XA 2
                    }
                }

                // No XA detected, so the sector might be a Mode 2 standard sector
                // Checking if it's a gap sector
                mLogger->trace("The sector might be a non XA Mode 2 sector.");
                return ST_MODE2;
            }

            // Data sector detected but was not possible to determine the mode. Maybe is a copy protection sector.
            mLogger->warn("Unable to determine the type of sector. Unknown sector mode returned.");
            return ST_MODEX;
        }
        else
        {
            // Sector is not recognized, so might be a CDDA sector
            mLogger->trace("Sync data not detected. Sector will be RAW (a.k.a CDDA). Checking if it's a GAP.");
            if (is_gap(inputSector, 0x930))
            {
                mLogger->debug("The sector is a CDDA GAP.");
                return ST_CDDA_GAP;
            }
            else
            {
                mLogger->debug("The sector is a CDDA.");
                return ST_CDDA;
            }
        }

        mLogger->warn("Unable to determine the sector type. Report this to the developer because this might not happen.");
        return ST_UNKNOWN;
    }
}