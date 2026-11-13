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

#include "formehudlayout.h"
#include <cassert>
int main()
{
    for (int width = 320; width <= 3840; width += 16)
        for (int digits : {8, 60, 120, 240, 400})
        {
            const auto header = ForMeHUD::header(width, digits);
            assert(header.balanceLeft >= 0 && header.balanceWidth >= 96);
            assert(header.balanceLeft + header.balanceWidth <= width - 148);
            if (header.showBrand) assert(header.balanceLeft > 168);
            if (header.showLocation)
                assert(header.locationLeft + header.locationWidth + 16 <= header.balanceLeft);
            if (header.showConnection)
                assert(header.connectionLeft + 148 <= header.balanceLeft);
        }
    const auto reference = ForMeHUD::layout(1672, 940, false, 0, 588, 92);
    assert(reference.chatWidth == 380 && reference.chatHeight == 204 && reference.chatBottom == 24);
    const auto emptyDock = ForMeHUD::layout(580, 420, false, 0, 0, 0);
    assert(emptyDock.chatBottom == 24);
    for (int width = 320; width <= 3840; width += 16)
        for (int height = 360; height <= 2160; height += 24)
            for (bool collapsed : {false, true})
            for (int extra : {0, 32, 80, 112})
            for (int dockTop : {92, 153})
            {
                const int dockLeft = std::max(12, (width - 496) / 2);
                const auto value = ForMeHUD::layout(width, height, collapsed, extra, dockLeft, dockTop);
                assert(value.chatWidth <= width - 24);
                assert(value.chatHeight >= 96);
                assert(value.chatBottom + value.chatHeight <= height - 12);
                const bool horizontalGap = 12 + value.chatWidth + 16 <= dockLeft;
                const bool verticalGap = value.chatBottom >= dockTop + 12;
                assert(horizontalGap || verticalGap);
                const auto other = ForMeHUD::layout(width, height, !collapsed, 0, dockLeft, dockTop);
                assert(value.chatBottom == other.chatBottom && value.chatWidth == other.chatWidth);
            }
}
