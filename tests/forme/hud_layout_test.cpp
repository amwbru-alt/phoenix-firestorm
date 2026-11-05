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
    const auto reference = ForMeHUD::layout(1672, 940, false);
    assert(reference.dockWidth == 496 && reference.dockLeft == 588);
    assert(reference.chatWidth == 380 && reference.chatHeight == 204 && reference.chatBottom == 24);
    for (int width = 320; width <= 3840; width += 16)
        for (int height = 360; height <= 2160; height += 24)
            for (bool collapsed : {false, true})
            {
                const auto value = ForMeHUD::layout(width, height, collapsed);
                assert(value.dockLeft >= 12 && value.dockLeft + value.dockWidth <= width - 12);
                assert(value.chatWidth <= width - 24);
                assert(value.chatHeight >= 96);
                assert(value.chatBottom + value.chatHeight <= height - 12);
                const bool horizontalGap = 12 + value.chatWidth + 16 <= value.dockLeft;
                const bool verticalGap = value.chatBottom >= value.dockBottom + 76 + 12;
                assert(horizontalGap || verticalGap);
                const auto other = ForMeHUD::layout(width, height, !collapsed);
                assert(value.chatBottom == other.chatBottom && value.chatWidth == other.chatWidth);
                assert(value.dockLeft == other.dockLeft && value.dockBottom == other.dockBottom);
            }
}
