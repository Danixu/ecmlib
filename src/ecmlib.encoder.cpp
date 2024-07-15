#include "ecmlib.encoder.hpp"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    encoder::encoder(optimizations opt) : base(opt)
    {
        m_logger->debug("Initializing encoder class.");

        m_logger->debug("Finished the encoder class inizialization.");
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
        m_logger->trace("Loading data into the buffer.");
        if (toRead > 2352)
        {
            // passed data is too big
            m_logger->error("The provided data is more than a sector.");
            return STATUS_ERROR_TOO_MUCH_DATA;
        }
        else if (toRead < 2352)
        {
            // For now the module doesn't accepts incremental loading
            m_logger->error("The provided data is less than a sector ({} bytes).", toRead);
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Clear the input sector data
        m_logger->trace("Clearing the buffer data");
        _input_sector = {0};
        // Copy the data to the input block
        m_logger->trace("Copying the data to the buffer");
        std::memcpy(_input_sector.data(), buffer, toRead);
        _input_sector_size = toRead;

        // Try to determine the sector type only if a full sector is provided
        if (toRead == 2352)
        {
            m_logger->debug("Determining the sector type");
            _sector_type = detect();
            m_logger->debug("Detected a sector of the type {}.", (uint8_t)_sector_type);
        }

        m_logger->trace("Data loaded correctly.");
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
        m_logger->debug("Optimizing the sector...\nForce: {}\nOnly Data: {}", force, onlyData);
        // Check if the sector was loaded
        if (_input_sector_size == 0)
        {
            m_logger->error("There is no input data. Execute the load method to load it first.");
            return STATUS_ERROR_NO_DATA;
        }

        // The sector is Unknown, so there were no enough data to detect it.
        if (_sector_type == ST_UNKNOWN)
        {
            m_logger->error("Sector type is unknown. Maybe there is no enough data to determine the type.");
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Copy only the data
        if (onlyData)
        {
            m_logger->trace("OnlyData mode activated. Only data will be sent to the output.");
            // Copy the data block to the output
            if (_sector_type == ST_CDDA || _sector_type == ST_CDDA_GAP)
            {
                m_logger->trace("The sector is a CDDA sector");
                if (_sector_type == ST_CDDA || !(_optimizations & OO_REMOVE_GAP))
                {
                    m_logger->trace("The sector will be fully copied.");
                    std::copy(_input_sector.begin(), _input_sector.end(), _output_sector.begin());
                    _output_sector_size = 2352;
                }
                else
                {
                    m_logger->trace("No data will be copied.");
                    _output_sector_size = 0;
                }
            }
        }

        m_logger->debug("Optimization finished.");
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
        m_logger->trace("Checking {} bytes for a gap.", length);
        for (size_t i = 0; i < length; i++)
        {
            if ((sector[i]) != 0x00)
            {
                m_logger->trace("Received data is not a GAP.");
                return false; // Sector contains data, so is not a GAP
            }
        }

        m_logger->trace("Received data is a GAP.");
        return true;
    }

    sector_type encoder::detect()
    {
        m_logger->trace("Detecting the sector type.");
        if (
            _input_sector[0x000] == (char)0x00 && // sync (12 bytes)
            _input_sector[0x001] == (char)0xFF &&
            _input_sector[0x002] == (char)0xFF &&
            _input_sector[0x003] == (char)0xFF &&
            _input_sector[0x004] == (char)0xFF &&
            _input_sector[0x005] == (char)0xFF &&
            _input_sector[0x006] == (char)0xFF &&
            _input_sector[0x007] == (char)0xFF &&
            _input_sector[0x008] == (char)0xFF &&
            _input_sector[0x009] == (char)0xFF &&
            _input_sector[0x00A] == (char)0xFF &&
            _input_sector[0x00B] == (char)0x00)
        {
            m_logger->trace("Sync data detected... Sector is a data sector.");
            // Sector is a MODE1/MODE2 sector
            if (
                _input_sector[0x00F] == (char)0x01 && // mode (1 byte)
                _input_sector[0x814] == (char)0x00 && // reserved (8 bytes)
                _input_sector[0x815] == (char)0x00 &&
                _input_sector[0x816] == (char)0x00 &&
                _input_sector[0x817] == (char)0x00 &&
                _input_sector[0x818] == (char)0x00 &&
                _input_sector[0x819] == (char)0x00 &&
                _input_sector[0x81A] == (char)0x00 &&
                _input_sector[0x81B] == (char)0x00)
            {
                m_logger->trace("Sector is a MODE1 sector. Checking EDC...");
                //  The sector is surelly MODE1 but we will check the EDC
                if (
                    ecc_check_sector(
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0xC),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x10),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x81C)) &&
                    edc_compute(_input_sector.data(), 0x810) == get32lsb(_input_sector.data() + 0x810))
                {
                    m_logger->trace("Mode 1 sector detected. Determining if it's a GAP.");
                    if (is_gap(_input_sector.data() + 0x010, 0x800))
                    {
                        m_logger->trace("The sector is at Mode 1 GAP.");
                        return ST_MODE1_GAP;
                    }
                    else
                    {
                        m_logger->trace("The sector is at Mode 1.");
                        return ST_MODE1; // Mode 1
                    }
                }

                // If EDC doesn't match, then the sector is damaged. It can be a protection method, so will be threated as RAW.
                m_logger->trace("The EDC cannot be verified, so the sector will be threated as RAW.");
                return ST_MODE1_RAW;
            }
            else if (
                _input_sector[0x00F] == (char)0x02 // mode 2 (1 byte)
            )
            {
                //  The sector is MODE2, and now we will detect what kind
                //
                // Might be Mode 2, XA 1 or 2
                //
                m_logger->trace("Mode 2 sector detected. Determining if XA 1 or XA 2.");
                if (
                    ecc_check_sector(
                        zeroaddress,
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x10),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x81C)) &&
                    edc_compute(_input_sector.data(), 0x808) == get32lsb(_input_sector.data() + 0x818))
                {
                    m_logger->trace("Mode 2 XA 1 detected. Checking if it's a GAP.");
                    if (is_gap(_input_sector.data() + 0x018, 0x800))
                    {
                        m_logger->trace("The sector is at Mode 2 XA 1 GAP.");
                        return ST_MODE2_XA1_GAP;
                    }
                    else
                    {
                        m_logger->trace("The sector is at Mode 2 XA 1.");
                        return ST_MODE2_XA1; //  Mode 2, XA 1
                    }
                }
                //
                // Might be Mode 2, XA 2
                //
                if (
                    edc_compute(_input_sector.data(), 0x91C) == get32lsb(_input_sector.data() + 0x92C))
                {
                    m_logger->trace("Mode 2 XA 2 detected. Checking if it's a GAP.");
                    if (is_gap(_input_sector.data() + 0x018, 0x914))
                    {
                        m_logger->trace("The sector is at Mode 2 XA 2 GAP.");
                        return ST_MODE2_XA2_GAP;
                    }
                    else
                    {
                        m_logger->trace("The sector is at Mode 2 XA 2.");
                        return ST_MODE2_XA2; // Mode 2, XA 2
                    }
                }

                // No XA detected, so the sector might be a Mode 2 standard sector
                // Checking if it's a gap sector
                m_logger->trace("The sector might be a non XA Mode 2 sector. Determining if it's a GAP.");
                if (is_gap(_input_sector.data() + 0x010, 0x920))
                {
                    m_logger->trace("The sector is at Mode 2 GAP.");
                    return ST_MODE2_GAP;
                }
                else
                {
                    m_logger->trace("The sector is at Mode 2.");
                    return ST_MODE2;
                }
            }

            // Data sector detected but was not possible to determine the mode. Maybe is a copy protection sector.
            m_logger->trace("Unable to determine the type of sector. Unknown sector mode returned.");
            return ST_MODEX;
        }
        else
        {
            // Sector is not recognized, so might be a CDDA sector
            m_logger->trace("Sync data not detected. Sector will be RAW (a.k.a CDDA). Checking if it's a GAP.");
            if (is_gap(_input_sector.data(), 0x930))
            {
                m_logger->trace("The sector is a CDDA GAP.");
                return ST_CDDA_GAP;
            }
            else
            {
                m_logger->trace("The sector is a CDDA.");
                return ST_CDDA;
            }
        }

        m_logger->trace("Unable to determine the sector type. Report this to the developer because this might not happen.");
        return ST_UNKNOWN;
    }
}