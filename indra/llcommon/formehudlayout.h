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
struct HeaderLayout
{
    int balanceLeft, balanceWidth, locationLeft, locationWidth, connectionLeft;
    bool showBrand, showLocation, showConnection;
};
inline HeaderLayout header(int width, int balanceTextWidth)
{
    const int balanceWidth = std::clamp(balanceTextWidth + 38, 96, std::min(180, std::max(96, width / 3)));
    const int balanceLeft = std::max(0, width - 148 - balanceWidth);
    const bool location = width >= 620;
    const bool connection = width >= 940;
    const int locationWidth = location ? std::min(200, std::max(0, balanceLeft - 200 - (connection ? 160 : 0))) : 0;
    return {balanceLeft, balanceWidth, 184, locationWidth, 196 + locationWidth,
        balanceLeft >= 180, location, connection};
}
struct Layout
{
    int chatWidth, chatHeight, chatBottom;
};
inline Layout layout(int width, int height, bool collapsed, int extraHeight, int dockLeft, int dockTop)
{
    const int chatWidth = std::min(380, std::max(0, width - 24));
    const int bottom = dockTop <= 0 || dockLeft >= 12 + chatWidth + 16 ? 24 : dockTop + 12;
    return {chatWidth,
        std::min((collapsed ? 112 : 204) + std::clamp(extraHeight, 0, 112), std::max(0, height - bottom - 12)), bottom};
}
}
#endif
