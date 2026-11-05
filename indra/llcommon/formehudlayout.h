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

#ifndef LL_FORMEHUDLAYOUT_H
#define LL_FORMEHUDLAYOUT_H
#include <algorithm>
namespace ForMeHUD
{
struct Layout
{
    int dockLeft, dockWidth, dockBottom, chatWidth, chatHeight, chatBottom;
};
inline Layout layout(int width, int height, bool collapsed)
{
    const int dockWidth = std::min(496, std::max(0, width - 24));
    const int dockLeft = (width - dockWidth) / 2;
    const int chatWidth = std::min(380, std::max(0, width - 24));
    const int bottom = dockLeft >= 12 + chatWidth + 16 ? 24 : 112;
    return {dockLeft, dockWidth, 24, chatWidth,
        std::min(collapsed ? 112 : 204, std::max(0, height - bottom - 12)), bottom};
}
}
#endif
