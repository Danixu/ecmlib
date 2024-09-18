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

#include "ecmlib.base.hpp"
#include <cstring>
#include <utility>

#ifndef __ECMLIB_ENCODER_H__
#define __ECMLIB_ENCODER_H__

struct sector_data_link
{
    char *dataPosition = nullptr;
    uint16_t dataSize = 0;
};

namespace ecmlib
{
    class encoder : public base
    {
    public:
        // Public Methods
        encoder();
        sector_type get_sector_type(char *buffer);
        status_code encode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer, uint16_t outBufferSize, uint16_t &encodedDataSize, optimizations opts, bool force = false);
        sector_type get_encoded_sector_type();
        optimizations get_encoded_optimizations();

    private:
        // ECM variables
        const uint8_t zeroaddress[4] = {0, 0, 0, 0};
        sector_data_link _sectorDataLink;
        sector_type _lastEncodedType = sector_type::ST_UNKNOWN;
        optimizations _lastOptimizations = optimizations::OO_NONE;

        // ecm tools inline functions
        static inline uint32_t get32lsb(const char *src)
        {
            return (uint32_t)(static_cast<uint8_t>(src[0]) << 0 |
                              static_cast<uint8_t>(src[1]) << 8 |
                              static_cast<uint8_t>(src[2]) << 16 |
                              static_cast<uint8_t>(src[3]) << 24);
        }

        // Methods
        bool inline is_gap(
            char *sector,
            size_t length);
        uint16_t get_encoded_size(sector_type sectorType, optimizations opts);
        status_code check_optimizations(char *buffer, optimizations opts);
    };
}

#endif