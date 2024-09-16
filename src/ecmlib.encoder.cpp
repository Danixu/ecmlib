#include "ecmlib.encoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new ecmlib::encoder object to process the CD-ROM sectors
     *
     */
    encoder::encoder() : base()
    {
        mLogger->debug("Initializing encoder class.");

        mLogger->debug("Finished the encoder class inizialization.");
    }

    /**
     * @brief Optimize the sector provided by the inBuffer argument and store the data into the outBuffer
     *
     * @param inBuffer The input buffer in char *. Must contains the full sector (2352 bytes)
     * @param inBufferSize The input buffer size.
     * @param outBuffer The output buffer where the data will be stored. The recommended size is at least 2352 bytes for full sector copy.
     * @param outBufferSize The size of the output buffer.
     * @param encodedDataSize OUTPUT: The variable to store the used space in the output buffer (a.k.a encoded sector size).
     * @param opts Encoding optimizations to use in the encoding process
     * @param force Force the optimizations in the sector without check if they are possible
     * @return status_code
     */
    status_code encoder::encode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer,
                                       uint16_t outBufferSize, uint16_t &encodedDataSize, optimizations opts, bool force)
    {
        mLogger->debug("Encoding the sector...");
        // Check if the sector was loaded
        if (inBufferSize < 2352)
        {
            mLogger->error("There is no enough input data. Load more data into the buffer first.");
            return status_code::STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Detect the sector type
        mLogger->trace("Detecting the sector type");
        _lastEncodedType = get_sector_type(inBuffer);

        // Check the optimizations if force = false
        if (!force)
        {
            mLogger->trace("Checking the applicable optimizations for the sector");
            check_optimizations(inBuffer, opts);
        }

        // Check the output buffer space
        mLogger->trace("Checking the required size to see if fits the output buffer.");
        uint16_t encodedEstimatedSize = get_encoded_size(_lastEncodedType, _lastOptimizations);
        if (encodedEstimatedSize > outBufferSize)
        {
            mLogger->error("There is no enough space in the output buffer. Estimated: {} - Current: {}.", encodedEstimatedSize, outBufferSize);
            return status_code::STATUS_ERROR_NO_ENOUGH_BUFFER_SPACE;
        }

        mLogger->trace("Encoding the sector.");
        encodedDataSize = 0;
        //
        //
        // SYNC Data
        //
        //
        if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_SYNC) &&
            (_lastEncodedType != sector_type::ST_CDDA && _lastEncodedType != sector_type::ST_CDDA_GAP))
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
        if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_MSF) &&
            (_lastEncodedType != sector_type::ST_CDDA && _lastEncodedType != sector_type::ST_CDDA_GAP))
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
        if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_MODE) &&
            (_lastEncodedType != sector_type::ST_CDDA && _lastEncodedType != sector_type::ST_CDDA_GAP))
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
        if (_lastEncodedType == sector_type::ST_MODE2_XA_GAP ||
            _lastEncodedType == sector_type::ST_MODE2_XA1 ||
            _lastEncodedType == sector_type::ST_MODE2_XA1_GAP ||
            _lastEncodedType == sector_type::ST_MODE2_XA2 ||
            _lastEncodedType == sector_type::ST_MODE2_XA2_GAP)
        {
            // Only Mode 2 XA will have FLAGS
            if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_REDUNDANT_FLAG))
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
        switch (_lastEncodedType)
        {
        case sector_type::ST_CDDA:
        case sector_type::ST_CDDA_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // CDDA sectors are fully raw, so all will be copied if it's not a GAP with the GAP optimization enabled.
            if (_lastEncodedType == sector_type::ST_CDDA || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_GAP))
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

        case sector_type::ST_MODE1:
        case sector_type::ST_MODE1_RAW:
        case sector_type::ST_MODE1_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode1 sectors starts at 0x10 and ends at 0x80F
            if (_lastEncodedType == sector_type::ST_MODE1 || _lastEncodedType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_GAP))
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

        case sector_type::ST_MODE2:
        case sector_type::ST_MODE2_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode2 sectors starts at 0x10 and ends at 0x92F
            if (_lastEncodedType == sector_type::ST_MODE2 || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_GAP))
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

        case sector_type::ST_MODE2_XA1:
        case sector_type::ST_MODE2_XA1_GAP:
        // The unknown XA mode (GAP) will be threated as XA1 because is mainly used in PSX games
        case sector_type::ST_MODE2_XA_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode1 sectors starts at 0x18 and ends at 0x817
            if (_lastEncodedType == sector_type::ST_MODE2_XA1 || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_GAP))
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

        case sector_type::ST_MODE2_XA2:
        case sector_type::ST_MODE2_XA2_GAP:
            // Set the data position
            _sectorDataLink.dataPosition = outBuffer + encodedDataSize;
            // Mode2 XA2 sectors starts at 0x18 and ends at 0x92B
            if (_lastEncodedType == sector_type::ST_MODE2_XA2 || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_GAP))
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
        switch (_lastEncodedType)
        {
        case sector_type::ST_MODE1:
        case sector_type::ST_MODE1_RAW:
        case sector_type::ST_MODE1_GAP:
            // Mode1 EDC starts at 0x810 and ends at 0x813
            if (_lastEncodedType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x810, 0x4);
                encodedDataSize += 4;
            }

            break;

        case sector_type::ST_MODE2_XA1:
        case sector_type::ST_MODE2_XA1_GAP:
        case sector_type::ST_MODE2_XA_GAP:
            // Mode2 XA1 EDC starts at 0x818 and ends at 0x81B
            if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_EDC))
            {
                std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x818, 0x4);
                encodedDataSize += 4;
            }

            break;

        case sector_type::ST_MODE2_XA2:
        case sector_type::ST_MODE2_XA2_GAP:
            // Mode2 XA2 EDC starts at 0x92C and ends at 0x92F
            if (!static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_EDC))
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
        if ((_lastEncodedType == sector_type::ST_MODE1 ||
             _lastEncodedType == sector_type::ST_MODE1_GAP ||
             _lastEncodedType == sector_type::ST_MODE1_RAW) &&
            (_lastEncodedType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_BLANKS)))
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
        if ((_lastEncodedType == sector_type::ST_MODE1 ||
             _lastEncodedType == sector_type::ST_MODE1_RAW ||
             _lastEncodedType == sector_type::ST_MODE1_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA1 ||
             _lastEncodedType == sector_type::ST_MODE2_XA1_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA_GAP) &&
            (_lastEncodedType == sector_type::ST_MODE1_RAW || !static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_ECC)))
        {
            std::memcpy(outBuffer + encodedDataSize, inBuffer + 0x81C, 0x114);
            encodedDataSize += 0x114;
        }

        mLogger->debug("Optimization finished.");
        return status_code::STATUS_OK;
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
                        return sector_type::ST_MODE1_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 1.");
                        return sector_type::ST_MODE1; // Mode 1
                    }
                }

                // If EDC doesn't match, then the sector is damaged. It can be a protection method, so will be threated as RAW.
                mLogger->trace("The EDC cannot be verified, so the sector will be threated as RAW.");
                return sector_type::ST_MODE1_RAW;
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
                    return sector_type::ST_MODE2_GAP;
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
                    return sector_type::ST_MODE2_XA_GAP;
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
                        return sector_type::ST_MODE2_XA1_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 2 XA 1.");
                        return sector_type::ST_MODE2_XA1; //  Mode 2, XA 1
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
                        return sector_type::ST_MODE2_XA2_GAP;
                    }
                    else
                    {
                        mLogger->debug("The sector is at Mode 2 XA 2.");
                        return sector_type::ST_MODE2_XA2; // Mode 2, XA 2
                    }
                }

                // No XA detected, so the sector might be a Mode 2 standard sector
                // Checking if it's a gap sector
                mLogger->trace("The sector might be a non XA Mode 2 sector.");
                return sector_type::ST_MODE2;
            }

            // Data sector detected but was not possible to determine the mode. Maybe is a copy protection sector.
            mLogger->warn("Unable to determine the type of sector. Unknown sector mode returned.");
            return sector_type::ST_MODEX;
        }
        else
        {
            // Sector is not recognized, so might be a CDDA sector
            mLogger->trace("Sync data not detected. Sector will be RAW (a.k.a CDDA). Checking if it's a GAP.");
            if (is_gap(inputSector, 0x930))
            {
                mLogger->debug("The sector is a CDDA GAP.");
                return sector_type::ST_CDDA_GAP;
            }
            else
            {
                mLogger->debug("The sector is a CDDA.");
                return sector_type::ST_CDDA;
            }
        }
    }

    /**
     * @brief Returns the type of the last encoded sector.
     *
     * @return sector_type
     */
    sector_type encoder::get_encoded_sector_type()
    {
        return _lastEncodedType;
    }

    /**
     * @brief Get the optimizations used to encode the last sector.
     *
     * @return optimizations
     */
    optimizations encoder::get_encoded_optimizations()
    {
        return _lastOptimizations;
    }

    /**
     * @brief Detects if the sector is a gap or contains data
     *
     * @param sector (char*) Stream containing all the bytes which will be compared
     * @param length (size_t) Length of the stream to check
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

    uint16_t encoder::get_encoded_size(sector_type sectorType, optimizations opts)
    {
        uint16_t outSize = 0;
        switch (sectorType)
        {
        case sector_type::ST_CDDA:
        case sector_type::ST_CDDA_GAP:
            mLogger->trace("CDDA Detected.");
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_GAP) || sectorType == sector_type::ST_CDDA)
            {
                outSize = 2352;
            }
            else
            {
                outSize = 0;
            }
            break;

        case sector_type::ST_MODE1:
        case sector_type::ST_MODE1_GAP:
        case sector_type::ST_MODE1_RAW:
            mLogger->trace("MODE1 Detected.");
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_SYNC))
                outSize += 0xC;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MSF))
                outSize += 0x3;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MODE))
                outSize += 0x1;
            // Data is copied unless is GAP
            if (sectorType != sector_type::ST_MODE1_GAP)
                outSize += 0x800;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC) || sectorType == sector_type::ST_MODE1_RAW)
                outSize += 0x4;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_BLANKS) || sectorType == sector_type::ST_MODE1_RAW)
                outSize += 0x8;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_ECC) || sectorType == sector_type::ST_MODE1_RAW)
                outSize += 0x114;
            break;

        case sector_type::ST_MODE2:
        case sector_type::ST_MODE2_GAP:
            mLogger->trace("MODE2 Detected.");
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_SYNC))
                outSize += 0xC;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MSF))
                outSize += 0x3;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MODE))
                outSize += 0x1;
            // Data is copied unless is GAP
            if (sectorType != sector_type::ST_MODE2_GAP)
                outSize += 0x920;
            break;

        case sector_type::ST_MODE2_XA1:
        case sector_type::ST_MODE2_XA1_GAP:
        case sector_type::ST_MODE2_XA_GAP:
            mLogger->trace("MODE2 XA1 Detected.");
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_SYNC))
                outSize += 0xC;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MSF))
                outSize += 0x3;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MODE))
                outSize += 0x1;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_REDUNDANT_FLAG))
                outSize += 0x8;
            else
                outSize += 4;
            // Data is copied unless is GAP
            if (sectorType == sector_type::ST_MODE2_XA1)
                outSize += 0x800;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC))
                outSize += 0x4;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_ECC))
                outSize += 0x114;
            break;

        case sector_type::ST_MODE2_XA2:
        case sector_type::ST_MODE2_XA2_GAP:
            mLogger->trace("MODE2 XA2 Detected.");
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_SYNC))
                outSize += 0xC;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MSF))
                outSize += 0x3;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_MODE))
                outSize += 0x1;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_REDUNDANT_FLAG))
                outSize += 0x8;
            else
                outSize += 4;
            // Data is copied unless is GAP
            if (sectorType == sector_type::ST_MODE2_XA2)
                outSize += 0x914;
            if (!static_cast<uint8_t>(opts & optimizations::OO_REMOVE_EDC))
                outSize += 0x4;
            break;

        default:
            break;
        }

        return outSize;
    }

    status_code encoder::check_optimizations(char *buffer, optimizations opts)
    {
        mLogger->trace("Starting the optimizations check.");
        // Set the optimizations to the received ones
        _lastOptimizations = opts;
        // SYNC, MSF and MODE optimizations can be always done. The decoder will receive that data.

        //
        //
        // OO_REMOVE_REDUNDANT_FLAG
        //
        //
        if (static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_REDUNDANT_FLAG) &&
            (_lastEncodedType == sector_type::ST_MODE2_XA_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA1 ||
             _lastEncodedType == sector_type::ST_MODE2_XA1_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA2 ||
             _lastEncodedType == sector_type::ST_MODE2_XA2_GAP))
        {
            mLogger->trace("Checking the REDUNDANT FLAGS optimization.");
            if (
                buffer[0x10] != buffer[0x14] ||
                buffer[0x11] != buffer[0x15] ||
                buffer[0x12] != buffer[0x16] ||
                buffer[0x13] != buffer[0x17])
            {
                mLogger->trace("The optimization REDUNDANT FLAGS is not applicable in this sector.");
                _lastOptimizations ^= optimizations::OO_REMOVE_REDUNDANT_FLAG;
            }
            else
            {
                mLogger->trace("The optimization REDUNDANT FLAGS is applicable.");
            }
        }

        //
        //
        // OO_REMOVE_EDC
        //
        //
        if (static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_EDC) &&
            (_lastEncodedType == sector_type::ST_MODE1 ||
             _lastEncodedType == sector_type::ST_MODE1_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA1 ||
             _lastEncodedType == sector_type::ST_MODE2_XA1_GAP ||
             _lastEncodedType == sector_type::ST_MODE2_XA2 ||
             _lastEncodedType == sector_type::ST_MODE2_XA2_GAP))
        {
            mLogger->trace("Checking the EDC optimization.");
            bool canBeDone = true;

            uint32_t calculatedEDC = 0;
            uint32_t recoveredEDC = 1;

            switch (_lastEncodedType)
            {
            case sector_type::ST_MODE1:
            case sector_type::ST_MODE1_GAP:
                calculatedEDC = edc_compute(buffer, 0x810);
                recoveredEDC = get32lsb(buffer + 0x810);
                if (calculatedEDC != recoveredEDC)
                {
                    canBeDone = false;
                }
                break;

            case sector_type::ST_MODE2_XA1:
            case sector_type::ST_MODE2_XA1_GAP:
            case sector_type::ST_MODE2_XA_GAP:
                calculatedEDC = edc_compute(buffer + 0x10, 0x808);
                recoveredEDC = get32lsb(buffer + 0x818);
                if (calculatedEDC != recoveredEDC)
                {
                    canBeDone = false;
                }
                break;

            case sector_type::ST_MODE2_XA2:
            case sector_type::ST_MODE2_XA2_GAP:
                calculatedEDC = edc_compute(buffer + 0x10, 0x91C);
                recoveredEDC = get32lsb(buffer + 0x92C);
                if (calculatedEDC != recoveredEDC)
                {
                    canBeDone = false;
                }
                break;

            default:
                break;
            }

            if (canBeDone)
            {
                mLogger->trace("The optimization EDC is not applicable in this sector.");
                _lastOptimizations ^= optimizations::OO_REMOVE_EDC;
            }
            else
            {
                mLogger->trace("The optimization EDC is applicable.");
            }
        }

        //
        //
        // OO_REMOVE_BLANKS
        //
        //
        if (static_cast<uint8_t>(_lastOptimizations & optimizations::OO_REMOVE_BLANKS) &&
            (_lastEncodedType == sector_type::ST_MODE1 ||
             _lastEncodedType == sector_type::ST_MODE1_GAP))
        {
            mLogger->trace("Checking the BLANKS optimization.");
            if (!is_gap(buffer + 815, 8))
            {
                mLogger->trace("The optimization BLANKS is not applicable in this sector.");
                _lastOptimizations ^= optimizations::OO_REMOVE_BLANKS;
            }
            else
            {
                mLogger->trace("The optimization BLANKS is applicable.");
            }
        }

        // If the ECC cannot be recovered, the sectors are detected as other types.

        return status_code::STATUS_OK;
    }
}