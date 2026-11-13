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

#ifndef FORME_REGION_PHOTO_H
#define FORME_REGION_PHOTO_H
#include "formegridpolicy.h"
#include <string>

namespace ForMeRegionPhoto
{
// A region cover is the published website image. Terrain tiles belong to the map canvas.
inline std::string url(std::string path)
{
    const std::string origin = ForMeGrid::LoginURI;
    if (path.starts_with(origin)) path = "/" + path.substr(origin.size());
    if (!path.starts_with("/web/assets/img/") || path.find("..") != std::string::npos ||
        path.find("//") != std::string::npos ||
        path.find_first_not_of("/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.") != std::string::npos)
        return {};
    return origin + path.substr(1);
}
}
#endif
