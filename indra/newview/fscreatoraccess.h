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

#ifndef FS_CREATOR_ACCESS_H
#define FS_CREATOR_ACCESS_H
#include "formecreatorpolicy.h"
#include "fsavatarbadges.h"
#include "llagent.h"
#include "llstartup.h"
#include "llviewerregion.h"
namespace ForMeCreatorAccess
{
inline bool allowed()
{
    // Use the current authenticated avatar's simulator metadata, never a label,
    // friend role, god mode, saved setting or another avatar's badge.
    if (LLStartUp::getStartupState() != STATE_STARTED) return false;
    LLViewerRegion* region = gAgent.getRegion();
    LLViewerObject* self = gObjectList.findObject(gAgent.getID());
    if (!region || !self || self->isDead() || self->getRegion() != region || gAgent.getID().isNull()) return false;
    const ForMeBadge::Data badge = ForMeAvatarBadges::get(gAgent.getID());
    return ForMeCreator::allowed(badge.valid, badge.id);
}
}
#endif
