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

#ifndef __ECMLIB_ENCODER_H__
#define __ECMLIB_ENCODER_H__

namespace ecmlib
{
    struct encoded_sector
    {
        char *dataPosition = nullptr;
        uint16_t dataSize = 0;
        char *optimizedSectorData = nullptr;
        uint16_t optimizedSectorSize = 0;
        optimizations realOptimizations = optimizations::OO_NONE;
    };

    class encoder : public base
    {
    public:
        // Public Methods
        encoder(optimizations opt);
        ~encoder();
        status_code load(char *buffer, uint16_t toRead);
        status_code optimize(bool force = false);
        sector_type inline get_sector_type() { return _sectorType; };
        encoded_sector inline get_encoded_sector() { return _sector; };

    private:
        // ECM variables
        const uint8_t zeroaddress[4] = {0, 0, 0, 0};
        encoded_sector _sector;
        // Methods
        bool inline is_gap(
            char *sector,
            size_t length);
        sector_type detect();
    };
}

#endif