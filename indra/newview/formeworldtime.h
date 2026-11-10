/**
 * @file
 * @brief
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2026, ForMe.World contributors.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

/** Host-clock metadata received only over the current region's trusted capabilities. */
#ifndef FORME_WORLD_TIME_H
#define FORME_WORLD_TIME_H
#include "llsd.h"
#include "llstring.h"
#include "lltimer.h"
#include <cmath>
#include <ctime>
#include <limits>

namespace ForMeWorldTime
{
inline void apply(const LLSD& clock)
{
    if (!clock.isMap() || clock["version"].asInteger() != 1 || clock["hour_cycle"].asInteger() != 24
        || !clock["utc_offset_seconds"].isInteger() || !clock["unix_time"].isReal()) return;
    const S32 offset = clock["utc_offset_seconds"].asInteger();
    const double instant = clock["unix_time"].asReal();
    const double delta = std::floor(instant) - static_cast<double>(std::time(nullptr));
    if (offset < -50400 || offset > 50400 || !std::isfinite(instant)
        || delta < std::numeric_limits<S32>::min() || delta > std::numeric_limits<S32>::max()) return;
    LLStringOps::setForMeTimeOffset(offset);
    gUTCOffset = static_cast<S32>(delta);
}
}
#endif

