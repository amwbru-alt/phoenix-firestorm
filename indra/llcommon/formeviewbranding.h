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

#ifndef FORME_VIEW_BRANDING_H
#define FORME_VIEW_BRANDING_H
#include <string>
namespace ForMeBranding
{
inline constexpr const char* Name = "ForMe.World";
inline bool legacyName(const std::string& value)
{
    std::string name;
    for (unsigned char c : value)
    {
        if (c == ' ' || c == '-' || c == '_') continue;
        name += c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
    return name == "secondlife" || name == "opensim" || name == "opensimulator" || name == "firestorm";
}
inline std::string systemName(const std::string& value) { return legacyName(value) ? Name : value; }
}
#endif
