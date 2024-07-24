#include "ecmlib.encoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    encoder::encoder(optimizations opt) : base(opt)
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
     * @brief Loads a sector into the buffer to be processed
     *
     * @return Returns a ecmlib::sector_type value with the detected sector type.
     */
    status_code encoder::load(char *buffer, uint16_t toRead)
    {
        mLogger->debug("Loading data into the buffer.");
        if (toRead > 2352)
        {
            // passed data is too big
            mLogger->error("The provided data is more than a sector.");
            return STATUS_ERROR_TOO_MUCH_DATA;
        }
        else if (toRead < 2352)
        {
            // For now the module doesn't accepts incremental loading
            mLogger->error("The provided data is less than a sector ({} bytes).", toRead);
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Clear the input sector data
        mLogger->trace("Clearing the buffer data and the sector type");
        std::fill(_inputSector.begin(), _inputSector.end(), 0);
        // Set the type to Unknown
        _sectorType = ST_UNKNOWN;
        // Copy the data to the input block
        mLogger->trace("Copying the data to the buffer");
        std::memcpy(_inputSector.data(), buffer, toRead);
        _inputSectorSize = toRead;

        // Try to determine the sector type only if a full sector is provided
        // if (toRead == 2352)
        //{
        mLogger->trace("Determining the sector type");
        _sectorType = detect();
        mLogger->trace("Detected a sector of the type {}.", (uint8_t)_sectorType);
        //}

        mLogger->debug("Data loaded correctly.");
        return STATUS_OK;
    }

    /**
     * @brief Optimize the sector and store the data into the internal buffer
     *
     * @param force Forces the optimization even if to recover the original sector is not possible
     * @param onlyData Remove all except the data from the sector
     * @return status_code Status code
     */
    status_code encoder::optimize(bool force, bool onlyData)
    {
        mLogger->debug("Optimizing the sector...\nForce: {}\nOnly Data: {}", force, onlyData);
        // Check if the sector was loaded
        if (_inputSectorSize == 0)
        {
            mLogger->error("There is no input data. Execute the load method to load it first.");
            return STATUS_ERROR_NO_DATA;
        }

        // The sector is Unknown, so there were no enough data to detect it.
        if (_sectorType == ST_UNKNOWN)
        {
            mLogger->error("Sector type is unknown. Maybe there is no enough data to determine the type.");
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Copy only the data
        if (onlyData)
        {
            mLogger->trace("OnlyData mode activated. Only data will be sent to the output.");
            // Any GAP sector when the optimization OO_REMOVE_GAP is enabled, must be 0
            if ((_optimizations & OO_REMOVE_GAP) &&
                (_sectorType == ST_CDDA_GAP ||
                 _sectorType == ST_MODE1_GAP ||
                 _sectorType == ST_MODE2_GAP ||
                 _sectorType == ST_MODE2_XA_GAP ||
                 _sectorType == ST_MODE2_XA1_GAP ||
                 _sectorType == ST_MODE2_XA2_GAP))
            {
                mLogger->trace("The sector is a GAP sector with the OO_REMOVE_GAP optimization enabled, so no data will be copied");
                _outputSectorSize = 0;
                return STATUS_OK;
            }
            // Sector type CDDA
            if (_sectorType == ST_CDDA || _sectorType == ST_CDDA_GAP)
            {
                mLogger->trace("The sector is a CDDA sector, so will be fully copied.");
                std::copy(_inputSector.begin(), _inputSector.end(), _outputSector.begin());
                _outputSectorSize = 2352;
            }
            // Sector type Mode1
            if (_sectorType == ST_MODE1 || _sectorType == ST_MODE1_GAP)
            {
                mLogger->trace("The sector is a Mode1 sector. Data between 0x10 and 0x80F will be copied");
                std::copy(_inputSector.begin() + 0x10, _inputSector.begin() + 0x80F, _outputSector.begin());
                _outputSectorSize = 0x80F - 0x10;
            }
            // ToDo:
            // Sector type Mode1 RAW
            if (_sectorType == ST_MODE1_RAW)
            {
                mLogger->trace("The sector is a Mode1 RAW sector. TODO!");
            }
            // Sector type Mode2
            if (_sectorType == ST_MODE2 || _sectorType == ST_MODE2_GAP)
            {
                mLogger->trace("The sector is a Mode2 sector. Data between 0x10 and 0x92F will be copied");
                std::copy(_inputSector.begin() + 0x10, _inputSector.begin() + 0x92F, _outputSector.begin());
                _outputSectorSize = 0x92F - 0x10;
            }
            // Sector type Mode2
            if (_sectorType == ST_MODE2_XA_GAP)
            {
                mLogger->trace("The sector is a Mode2 XA GAP sector. Data between 0x19 and 0x92F will be copied");
                std::copy(_inputSector.begin() + 0x18, _inputSector.begin() + 0x92F, _outputSector.begin());
                _outputSectorSize = 0x92F - 0x18;
            }
            // Sector type Mode2 XA Gap
            if (_sectorType == ST_MODE2_XA_GAP)
            {
                mLogger->trace("The sector is a Mode2 XA GAP sector. Data between 0x19 and 0x92F will be copied");
                std::copy(_inputSector.begin() + 0x18, _inputSector.begin() + 0x92F, _outputSector.begin());
                _outputSectorSize = 0x92F - 0x18;
            }
            // Sector type Mode2 XA1
            if (_sectorType == ST_MODE2_XA1 || _sectorType == ST_MODE2_XA1_GAP)
            {
                mLogger->trace("The sector is a Mode2 XA1 sector. Data between 0x19 and 0x92F will be copied");
                std::copy(_inputSector.begin() + 0x18, _inputSector.begin() + 0x817, _outputSector.begin());
                _outputSectorSize = 0x817 - 0x18;
            }
            // Sector type Mode2 XA2
            if (_sectorType == ST_MODE2_XA2 || _sectorType == ST_MODE2_XA2_GAP)
            {
                mLogger->trace("The sector is a Mode2 XA2 sector. Data between 0x19 and 0x92F will be copied");
                std::copy(_inputSector.begin() + 0x18, _inputSector.begin() + 0x9BB, _outputSector.begin());
                _outputSectorSize = 0x9BB - 0x18;
            }
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
                mLogger->trace("Received data is not a GAP.");
                return false; // Sector contains data, so is not a GAP
            }
        }

        mLogger->trace("Received data is a GAP.");
        return true;
    }

    sector_type encoder::detect()
    {
        mLogger->debug("Detecting the sector type.");
        if (
            _inputSector[0x000] == (char)0x00 && // sync (12 bytes)
            _inputSector[0x001] == (char)0xFF &&
            _inputSector[0x002] == (char)0xFF &&
            _inputSector[0x003] == (char)0xFF &&
            _inputSector[0x004] == (char)0xFF &&
            _inputSector[0x005] == (char)0xFF &&
            _inputSector[0x006] == (char)0xFF &&
            _inputSector[0x007] == (char)0xFF &&
            _inputSector[0x008] == (char)0xFF &&
            _inputSector[0x009] == (char)0xFF &&
            _inputSector[0x00A] == (char)0xFF &&
            _inputSector[0x00B] == (char)0x00)
        {
            mLogger->trace("Sync data detected... Sector is a data sector.");
            // Sector is a MODE1/MODE2 sector
            if (
                _inputSector[0x00F] == (char)0x01 && // mode (1 byte)
                _inputSector[0x814] == (char)0x00 && // reserved (8 bytes)
                _inputSector[0x815] == (char)0x00 &&
                _inputSector[0x816] == (char)0x00 &&
                _inputSector[0x817] == (char)0x00 &&
                _inputSector[0x818] == (char)0x00 &&
                _inputSector[0x819] == (char)0x00 &&
                _inputSector[0x81A] == (char)0x00 &&
                _inputSector[0x81B] == (char)0x00)
            {
                mLogger->trace("Sector is a MODE1 sector. Checking EDC...");
                //  The sector is surelly MODE1 but we will check the EDC
                if (
                    ecc_check_sector(
                        reinterpret_cast<uint8_t *>(_inputSector.data() + 0xC),
                        reinterpret_cast<uint8_t *>(_inputSector.data() + 0x10),
                        reinterpret_cast<uint8_t *>(_inputSector.data() + 0x81C)) &&
                    edc_compute(_inputSector.data(), 0x810) == get32lsb(_inputSector.data() + 0x810))
                {
                    mLogger->trace("Mode 1 sector detected. Determining if it's a GAP.");
                    if (is_gap(_inputSector.data() + 0x010, 0x800))
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
                _inputSector[0x00F] == (char)0x02 // mode 2 (1 byte)
            )
            {
                //  The sector is MODE2, and now we will detect what kind
                //
                // Might be Mode 2, XA 1 or 2
                //
                mLogger->trace("Mode 2 sector detected. Determining if XA 1 or XA 2.");
                if (ecc_check_sector(zeroaddress,
                                     reinterpret_cast<uint8_t *>(_inputSector.data() + 0x10),
                                     reinterpret_cast<uint8_t *>(_inputSector.data() + 0x81C)) &&
                    edc_compute(_inputSector.data() + 0x10, 0x808) == get32lsb(_inputSector.data() + 0x818))
                {
                    mLogger->trace("Mode 2 XA 1 detected. Checking if it's a GAP.");
                    if (is_gap(_inputSector.data() + 0x018, 0x800))
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
                    edc_compute(_inputSector.data() + 0x10, 0x91C) == get32lsb(_inputSector.data() + 0x92C))
                {
                    mLogger->trace("Mode 2 XA 2 detected. Checking if it's a GAP.");
                    if (is_gap(_inputSector.data() + 0x018, 0x914))
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
                mLogger->trace("The sector might be a non XA Mode 2 sector. Determining if it's a GAP.");
                if (is_gap(_inputSector.data() + 0x010, 0x920))
                {
                    mLogger->debug("The sector is at Mode 2 GAP.");
                    return ST_MODE2_GAP;
                }
                else
                {
                    mLogger->debug("The sector is at Mode 2.");
                    return ST_MODE2;
                }
            }

            // Data sector detected but was not possible to determine the mode. Maybe is a copy protection sector.
            mLogger->warn("Unable to determine the type of sector. Unknown sector mode returned.");
            return ST_MODEX;
        }
        else
        {
            // Sector is not recognized, so might be a CDDA sector
            mLogger->trace("Sync data not detected. Sector will be RAW (a.k.a CDDA). Checking if it's a GAP.");
            if (is_gap(_inputSector.data(), 0x930))
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