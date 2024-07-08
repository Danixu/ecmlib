#include "ecmlib.h"

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    processor::processor(optimizations opt)
    {
        // Initialize the buffers
        _input_sector.resize(2352, 0);
        _output_sector.resize(2352, 0);
        // Set the optimizations
        _optimizations = opt;

        // Generate the ECM edc and ecc data
        size_t i;
        for (i = 0; i < 256; i++)
        {
            uint32_t edc = i;
            size_t j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
            ecc_f_lut[i] = j;
            ecc_b_lut[i ^ j] = i;
            for (j = 0; j < 8; j++)
            {
                edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
            }
            edc_lut[i] = edc;
        }
    }

    /**
     * @brief Object cleanup
     *
     */
    processor::~processor()
    {
    }

    /**
     * @brief Loads a sector into the buffer to be processed
     *
     * @return Returns a ecmlib::sector_type value with the detected sector type.
     */
    status_code processor::load(char *buffer, uint16_t toRead)
    {
        if (toRead > 2352)
        {
            // passed data is too big
            return STATUS_ERROR_TOO_MUCH_DATA;
        }

        // Clear the input sector data
        _input_sector = {0};
        // Copy the data to the input block
        std::memcpy(_input_sector.data(), buffer, toRead);
        _input_sector_size = toRead;

        // Try to determine the sector type only if a full sector is provided
        if (toRead == 2352)
        {
            _sector_type = detect();
        }

        return STATUS_OK;
    }

    /**
     * @brief Optimize the sector and store the data into the internal buffer
     *
     * @param force Forces the optimization even if to recover the original sector is not possible
     * @param onlyData Remove all except the data from the sector
     * @return status_code Status code
     */
    status_code processor::optimize(bool force, bool onlyData)
    {
        // Check if the sector was loaded
        if (_input_sector_size == 0)
        {
            return STATUS_ERROR_NO_DATA;
        }

        // The sector data was not enough to determine the sector type
        if (_sector_type == ST_UNKNOWN)
        {
            return STATUS_ERROR_NO_ENOUGH_DATA;
        }

        // Copy only the data
        if (onlyData)
        {
            // Copy the data block to the output
            if (_sector_type == ST_CDDA || _sector_type == ST_CDDA_GAP)
            {
                std::copy(_input_sector.begin(), _input_sector.end(), _output_sector.begin());
            }
        }

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
    bool inline processor::is_gap(char *sector, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            if ((sector[i]) != 0x00)
            {
                return false; // Sector contains data, so is not a GAP
            }
        }

        return true;
    }

    sector_type processor::detect()
    {
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
                //  The sector is surelly MODE1 but we will check the EDC
                if (
                    ecc_check_sector(
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0xC),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x10),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x81C)) &&
                    edc_compute(_input_sector.data(), 0x810) == get32lsb(_input_sector.data() + 0x810))
                {
                    if (is_gap(_input_sector.data() + 0x010, 0x800))
                    {
                        return ST_MODE1_GAP;
                    }
                    else
                    {
                        return ST_MODE1; // Mode 1
                    }
                }

                // If EDC doesn't match, then the sector is damaged. It can be a protection method, so will be threated as RAW.
                return ST_MODE1_RAW;
            }
            else if (
                _input_sector[0x00F] == (char)0x02 // mode (1 byte)
            )
            {
                //  The sector is MODE2, and now we will detect what kind
                //
                // Might be Mode 2, Form 1 or 2
                //
                if (
                    ecc_check_sector(
                        zeroaddress,
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x10),
                        reinterpret_cast<uint8_t *>(_input_sector.data() + 0x81C)) &&
                    edc_compute(_input_sector.data(), 0x808) == get32lsb(_input_sector.data() + 0x818))
                {
                    if (is_gap(_input_sector.data() + 0x018, 0x800))
                    {
                        return ST_MODE2_XA1_GAP;
                    }
                    else
                    {
                        return ST_MODE2_XA1; //  Mode 2, Form 1
                    }
                }
                //
                // Might be Mode 2, Form 2
                //
                if (
                    edc_compute(_input_sector.data(), 0x91C) == get32lsb(_input_sector.data() + 0x92C))
                {
                    if (is_gap(_input_sector.data() + 0x018, 0x914))
                    {
                        return ST_MODE2_XA2_GAP;
                    }
                    else
                    {
                        return ST_MODE2_XA2; // Mode 2, Form 2
                    }
                }

                // Checking if sector is MODE 2 without XA
                if (is_gap(_input_sector.data() + 0x010, 0x920))
                {
                    return ST_MODE2_GAP;
                }
                else
                {
                    return ST_MODE2;
                }
            }

            // Data sector detected but was not possible to determine the mode. Maybe is a copy protection sector.
            return ST_MODEX;
        }
        else
        {
            // Sector is not recognized, so might be a CDDA sector
            if (is_gap(_input_sector.data(), 0x930))
            {
                return ST_CDDA_GAP;
            }
            else
            {
                return ST_CDDA;
            }

            // If all sectors are NULL then the sector is a CDDA GAP
            return ST_CDDA_GAP;
        }

        return ST_UNKNOWN;
    }

    inline uint32_t processor::get32lsb(const char *src)
    {
        return (uint32_t)(static_cast<uint8_t>(src[0]) << 0 |
                          static_cast<uint8_t>(src[1]) << 8 |
                          static_cast<uint8_t>(src[2]) << 16 |
                          static_cast<uint8_t>(src[3]) << 24);
    }

    inline void processor::put32lsb(char *output, uint32_t value)
    {
        output[0] = (char)(value);
        output[1] = (char)(value >> 8);
        output[2] = (char)(value >> 16);
        output[3] = (char)(value >> 24);
    }

    inline uint32_t processor::edc_compute(
        const char *src,
        size_t size)
    {
        uint32_t edc = 0;
        for (; size; size--)
        {
            edc = (edc >> 8) ^ edc_lut[(edc ^ (*src++)) & 0xFF];
        }
        return edc;
    }

    int8_t processor::ecc_checkpq(
        const uint8_t *address,
        const uint8_t *data,
        size_t majorCount,
        size_t minorCount,
        size_t majorMult,
        size_t minorInc,
        const uint8_t *ecc)
    {
        size_t size = majorCount * minorCount;
        size_t major;
        for (major = 0; major < majorCount; major++)
        {
            size_t index = (major >> 1) * majorMult + (major & 1);
            uint8_t ecc_a = 0;
            uint8_t ecc_b = 0;
            size_t minor;
            for (minor = 0; minor < minorCount; minor++)
            {
                uint8_t temp;
                if (index < 4)
                {
                    temp = address[index];
                }
                else
                {
                    temp = data[index - 4];
                }
                index += minorInc;
                if (index >= size)
                {
                    index -= size;
                }
                ecc_a ^= temp;
                ecc_b ^= temp;
                ecc_a = ecc_f_lut[ecc_a];
            }
            ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
            if (
                ecc[major] != (ecc_a) ||
                ecc[major + majorCount] != (ecc_a ^ ecc_b))
            {
                return 0;
            }
        }
        return 1;
    }

    void processor::ecc_write_pq(
        const uint8_t *address,
        const uint8_t *data,
        size_t majorCount,
        size_t minorCount,
        size_t majorMult,
        size_t minorInc,
        uint8_t *ecc)
    {
        size_t size = majorCount * minorCount;
        size_t major;
        for (major = 0; major < majorCount; major++)
        {
            size_t index = (major >> 1) * majorMult + (major & 1);
            uint8_t ecc_a = 0;
            uint8_t ecc_b = 0;
            size_t minor;
            for (minor = 0; minor < minorCount; minor++)
            {
                uint8_t temp;
                if (index < 4)
                {
                    temp = address[index];
                }
                else
                {
                    temp = data[index - 4];
                }
                index += minorInc;
                if (index >= size)
                {
                    index -= size;
                }
                ecc_a ^= temp;
                ecc_b ^= temp;
                ecc_a = ecc_f_lut[ecc_a];
            }
            ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
            ecc[major] = (ecc_a);
            ecc[major + majorCount] = (ecc_a ^ ecc_b);
        }
    }

    int8_t processor::ecc_check_sector(
        const uint8_t *address,
        const uint8_t *data,
        const uint8_t *ecc)
    {
        return ecc_checkpq(address, data, 86, 24, 2, 86, ecc) &&       // P
               ecc_checkpq(address, data, 52, 43, 86, 88, ecc + 0xAC); // Q
    }

    void processor::ecc_write_sector(
        const uint8_t *address,
        const uint8_t *data,
        uint8_t *ecc)
    {
        ecc_write_pq(address, data, 86, 24, 2, 86, ecc);         // P
        ecc_write_pq(address, data, 52, 43, 86, 88, ecc + 0xAC); // Q
    }
}