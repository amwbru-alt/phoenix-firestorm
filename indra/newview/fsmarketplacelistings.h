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

/** ForMe.World native marketplace capability, bound to the current avatar. */
#ifndef FS_MARKETPLACE_LISTINGS_H
#define FS_MARKETPLACE_LISTINGS_H
#include "fscreatoraccess.h"
#include "formegridpolicy.h"
#include "llagent.h"
#include "llviewerregion.h"
namespace ForMeMarketplace
{
inline std::string capability()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!ForMeCreatorAccess::allowed() || !region) return "";
    const std::string cap = region->getCapability("DirectDelivery");
    return ForMeGrid::allowedServiceURI(cap) ? cap : "";
}
inline bool currentRequest(const std::string& url)
{
    const std::string cap = capability();
    return !cap.empty() && url.starts_with(cap + "/");
}
inline bool available() { return !capability().empty(); }
}
#endif
