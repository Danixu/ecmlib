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

#ifndef __ECMLIB_DECODER_H__
#define __ECMLIB_DECODER_H__

namespace ecmlib
{
    class decoder : public base
    {
    public:
        // Public Methods
        decoder();
        status_code decode_sector(char *inBuffer, uint16_t inBufferSize, char *outBuffer,
                                  uint16_t outBufferSize, sector_type sectorType, uint16_t sectorNumber, optimizations opts);

    private:
        // Logging
        std::shared_ptr<spdlog::logger> mLogger;

        // ECM variables
        const std::vector<uint8_t> zeroaddress = {0, 0, 0, 0};

        // ecm tools inline functions
        static inline void put32lsb(
            char *output,
            uint32_t value)
        {
            output[0] = (char)(value);
            output[1] = (char)(value >> 8);
            output[2] = (char)(value >> 16);
            output[3] = (char)(value >> 24);
        }

        // Methods
        std::vector<char> inline sector_to_time(uint32_t sectorNumber) const;
    };
}

#endif