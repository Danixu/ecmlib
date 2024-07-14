/*******************************************************************************
 *
 * Created by Daniel Carrasco at https://www.electrosoftcloud.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License 2.0.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License 2.0 for more details.
 *
 * You should have received a copy of the Mozilla Public License 2.0
 * along with this program.  If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 ******************************************************************************/

//////////////////////////////////////////////////////////////////
//
// Sector Detector class
//
// This class allows to detect the sector type of a CD-ROM sector
// From now is only able to detect the following sectors types:
//   * CDDA: If the sector type is unrecognized, will be threated as raw (like CDDA)
//   * CDDA_GAP: As above sector type, unrecognized type. The difference is that GAP is zeroed

////////////////////////////////////////////////////////////////////////////////
//
// Sector types
//
// CDDA
// -----------------------------------------------------
//        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0000h [---DATA...
// ...
// 0920h                                     ...DATA---]
// -----------------------------------------------------
//
// Mode 1
// -----------------------------------------------------
//        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0000h 00 FF FF FF FF FF FF FF FF FF FF 00 [-MSF -] 01
// 0010h [---DATA...
// ...
// 0800h                                     ...DATA---]
// 0810h [---EDC---] 00 00 00 00 00 00 00 00 [---ECC...
// ...
// 0920h                                      ...ECC---]
// -----------------------------------------------------
//
// Mode 2: This mode is not widely used
// -----------------------------------------------------
//        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0000h 00 FF FF FF FF FF FF FF FF FF FF 00 [-MSF -] 02
// 0010h [---DATA...
// ...
// 0920h                                     ...DATA---]
// -----------------------------------------------------
//
// Mode 2 (XA), form 1
// -----------------------------------------------------
//        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0000h 00 FF FF FF FF FF FF FF FF FF FF 00 [-MSF -] 02
// 0010h [--FLAGS--] [--FLAGS--] [---DATA...
// ...
// 0810h             ...DATA---] [---EDC---] [---ECC...
// ...
// 0920h                                      ...ECC---]
// -----------------------------------------------------
//
// Mode 2 (XA), form 2
// -----------------------------------------------------
//        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// 0000h 00 FF FF FF FF FF FF FF FF FF FF 00 [-MSF -] 02
// 0010h [--FLAGS--] [--FLAGS--] [---DATA...
// ...
// 0920h                         ...DATA---] [---EDC---]
// -----------------------------------------------------
//
// MSF:  Sector address, encoded as minutes:seconds:frames in BCD
// FLAGS: Used in Mode 2 (XA) sectors describing the type of sector; repeated
//        twice for redundancy
// DATA:  Area of the sector which contains the actual data itself
// EDC:   Error Detection Code
// ECC:   Error Correction Code
//
// First sector address looks like is always 00:02:00, so we will use this number on it

#include <stdint.h>
#include <vector>
#include "spdlog/spdlog.h"

#ifndef __ECMLIB_BASE_H__
#define __ECMLIB_BASE_H__

enum status_code : int8_t
{
    STATUS_ERROR_UNKNOWN_ERROR = -127,
    STATUS_ERROR_NO_DATA,        // The sector was not loaded in the library
    STATUS_ERROR_NO_ENOUGH_DATA, // The provided data is incomplete
    STATUS_ERROR_TOO_MUCH_DATA,  // Too much data provided to the lib

    STATUS_OK = 0
};

enum sector_type : uint8_t
{
    ST_UNKNOWN = 0,
    ST_CDDA,
    ST_CDDA_GAP,
    ST_MODE1,
    ST_MODE1_GAP,
    ST_MODE1_RAW,
    ST_MODE2,
    ST_MODE2_GAP,
    ST_MODE2_XA_GAP, // Detected in some games. The sector contains the XA flags, but is fully zeroed including the EDC/ECC data, then will be detected as non GAP Mode2 sector
    ST_MODE2_XA1,
    ST_MODE2_XA1_GAP,
    ST_MODE2_XA2,
    ST_MODE2_XA2_GAP,
    ST_MODEX
};

enum optimizations : uint8_t
{
    OO_NONE = 0,                       // Just copy the input. Surelly will not be used
    OO_REMOVE_SYNC = 1,                // Remove sync bytes (a.k.a first 12 bytes)
    OO_REMOVE_MSF = 1 << 1,            // Remove the MSF bytes
    OO_REMOVE_MODE = 1 << 2,           // Remove the MODE byte
    OO_REMOVE_BLANKS = 1 << 3,         // Remove the Mode 1 zeroed section of the sector
    OO_REMOVE_REDUNDANT_FLAG = 1 << 4, // Remove the redundant copy of FLAG bytes in Mode 2 XA sectors
    OO_REMOVE_ECC = 1 << 5,            // Remove the ECC
    OO_REMOVE_EDC = 1 << 6,            // Remove the EDC
    OO_REMOVE_GAP = 1 << 7             // If sector type is a GAP, remove the data

};
optimizations inline operator|(optimizations a, optimizations b)
{
    return static_cast<optimizations>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

namespace ecmlib
{
    class base
    {
    public:
        base(optimizations opt);
        ~base();
        virtual status_code load(char *buffer, uint16_t toRead);

    protected:
        // Variables
        std::vector<char> _input_sector;
        uint16_t _input_sector_size = 0;
        std::vector<char> _output_sector;
        uint16_t _output_sector_size = 0;
        sector_type _sector_type = sector_type::ST_UNKNOWN;
        optimizations _optimizations = optimizations::OO_NONE;

        // ecm tools inline functions
        static inline uint32_t get32lsb(
            const char *src)
        {
            return (uint32_t)(static_cast<uint8_t>(src[0]) << 0 |
                              static_cast<uint8_t>(src[1]) << 8 |
                              static_cast<uint8_t>(src[2]) << 16 |
                              static_cast<uint8_t>(src[3]) << 24);
        }

        static inline void put32lsb(
            char *output,
            uint32_t value)
        {
            output[0] = (char)(value);
            output[1] = (char)(value >> 8);
            output[2] = (char)(value >> 16);
            output[3] = (char)(value >> 24);
        }

        inline uint32_t edc_compute(
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

        // ecm tools
        int8_t ecc_checkpq(
            const uint8_t *address,
            const uint8_t *data,
            size_t major_count,
            size_t minor_count,
            size_t major_mult,
            size_t minor_inc,
            const uint8_t *ecc);
        int8_t ecc_check_sector(
            const uint8_t *address,
            const uint8_t *data,
            const uint8_t *ecc);
        void ecc_write_pq(
            const uint8_t *address,
            const uint8_t *data,
            size_t major_count,
            size_t minor_count,
            size_t major_mult,
            size_t minor_inc,
            uint8_t *ecc);
        void ecc_write_sector(
            const uint8_t *address,
            const uint8_t *data,
            uint8_t *ecc);

    private:
        // LUTs used for computing ECC/EDC
        uint8_t ecc_f_lut[256];
        uint8_t ecc_b_lut[256];
        uint32_t edc_lut[256];

        // Logging
        std::shared_ptr<spdlog::logger> m_logger;   
    };
}

#endif