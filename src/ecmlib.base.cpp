#include "ecmlib.base.hpp"
#include <spdlog/sinks/stdout_sinks.h>

namespace ecmlib
{
    /**
     * @brief Construct a new processor::processor object to process the CD-ROM sectors
     *
     */
    base::base()
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
        mLogger->debug("Initializing base class.");

        // Generate the ECM edc and ecc data
        mLogger->trace("Generating required ecc and edc data.");
        ecc_f_lut.resize(256);
        ecc_b_lut.resize(256);
        edc_lut.resize(256);
        for (size_t i = 0; i < 256; i++)
        {
            auto edc = (uint32_t)i;
            size_t j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
            ecc_f_lut[i] = (uint8_t)j;
            ecc_b_lut[i ^ j] = (uint8_t)i;
            for (j = 0; j < 8; j++)
            {
                edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
            }
            edc_lut[i] = edc;
        }

        mLogger->debug("Finished the base initialization.");
    }

    int8_t base::ecc_checkpq(
        const uint8_t *address,
        const uint8_t *data,
        size_t majorCount,
        size_t minorCount,
        size_t majorMult,
        size_t minorInc,
        const uint8_t *ecc) const
    {
        size_t size = majorCount * minorCount;
        for (size_t major = 0; major < majorCount; major++)
        {
            size_t index = (major >> 1) * majorMult + (major & 1);
            uint8_t ecc_a = 0;
            uint8_t ecc_b = 0;
            for (size_t minor = 0; minor < minorCount; minor++)
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
                ecc[major] != ecc_a ||
                ecc[major + majorCount] != (ecc_a ^ ecc_b))
            {
                return 0;
            }
        }
        return 1;
    }

    void base::ecc_write_pq(
        const uint8_t *address,
        const uint8_t *data,
        size_t majorCount,
        size_t minorCount,
        size_t majorMult,
        size_t minorInc,
        uint8_t *ecc) const
    {
        size_t size = majorCount * minorCount;
        for (size_t major = 0; major < majorCount; major++)
        {
            size_t index = (major >> 1) * majorMult + (major & 1);
            uint8_t ecc_a = 0;
            uint8_t ecc_b = 0;
            for (size_t minor = 0; minor < minorCount; minor++)
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
            ecc[major] = ecc_a;
            ecc[major + majorCount] = (ecc_a ^ ecc_b);
        }
    }

    int8_t base::ecc_check_sector(
        const uint8_t *address,
        const uint8_t *data,
        const uint8_t *ecc) const
    {
        return ecc_checkpq(address, data, 86, 24, 2, 86, ecc) &&       // P
               ecc_checkpq(address, data, 52, 43, 86, 88, ecc + 0xAC); // Q
    }

    void base::ecc_write_sector(
        const uint8_t *address,
        const uint8_t *data,
        uint8_t *ecc) const
    {
        ecc_write_pq(address, data, 86, 24, 2, 86, ecc);         // P
        ecc_write_pq(address, data, 52, 43, 86, 88, ecc + 0xAC); // Q
    }

    std::string base::logger_name()
    {
        return ecmLoggerName;
    }
}