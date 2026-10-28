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

/** ForMe.World distribution policy. No settings, URI or grid-file overrides. */
#ifndef FORME_GRID_POLICY_H
#define FORME_GRID_POLICY_H
#include <string>

namespace ForMeGrid
{
inline constexpr const char* LoginURI = "http://185.173.93.200:8002/";
inline constexpr const char* GridID = "185.173.93.200:8002";
inline constexpr const char* Host = "185.173.93.200";
inline constexpr const char* Label = "ForMe.World";
inline bool allowedLoginURI(const std::string& uri)
{
    return uri == LoginURI || uri == "http://185.173.93.200:8002";
}
inline bool allowedGrid(const std::string& grid)
{
    return grid == GridID || grid == Label || grid == "forme" || grid == "forme.world" || allowedLoginURI(grid);
}
inline bool allowedCredential(const std::string& name)
{
    const auto at = name.rfind('@');
    return at != std::string::npos && at > 0 && name.substr(at + 1) == GridID;
}
inline bool allowedSimulator(const std::string& host) { return host == Host; }
inline bool localRegion(const std::string& name)
{
    return name.find(':') == std::string::npos && name.find('@') == std::string::npos;
}
inline bool allowedServiceURI(const std::string& uri)
{
    for (unsigned char c : uri) if (c <= 32 || c == 127 || c == '\\') return false;
    std::size_t start;
    if (uri.compare(0, 7, "http://") == 0) start = 7;
    else if (uri.compare(0, 8, "https://") == 0) start = 8;
    else return false;
    const auto end = uri.find_first_of("/?#", start);
    const std::string authority = uri.substr(start, end == std::string::npos ? end : end - start);
    if (authority == Host) return uri.find('#') == std::string::npos;
    const std::string prefix = std::string(Host) + ":";
    if (authority.compare(0, prefix.size(), prefix) != 0) return false;
    const std::string port = authority.substr(prefix.size());
    if (port.empty() || port.size() > 5) return false;
    unsigned value = 0;
    for (char c : port) { if (c < '0' || c > '9') return false; value = value * 10 + (c - '0'); }
    return value > 0 && value <= 65535 && uri.find('#') == std::string::npos;
}
}
#endif
