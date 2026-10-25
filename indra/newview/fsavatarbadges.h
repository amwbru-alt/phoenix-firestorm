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

#ifndef FS_AVATAR_BADGES_H
#define FS_AVATAR_BADGES_H
#include "formeavatarbadge.h"
#include "formegridpolicy.h"
#include "llchat.h"
#include "llnamevalue.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "v4color.h"
#include "rlvactions.h"

namespace ForMeAvatarBadges
{
inline LLColor4 color(const std::array<float, 4>& value) { return LLColor4(value[0], value[1], value[2], value[3]); }
inline ForMeBadge::Data get(const LLUUID& id)
{
    LLViewerObject* object = gObjectList.findObject(id);
    if (!object || !object->isAvatar() || !object->getRegion()) return {};
    LLViewerRegion* region = object->getRegion();
    LLSD features;
    region->getSimulatorFeatures(features);
    const LLSD& advertised = features;
    if (!ForMeBadge::eligible(true, ForMeGrid::allowedSimulator(region->getHost().getIPString()),
            advertised["ForMeAvatarBadges"]["version"].asInteger(), false, false)) return {};
    const auto field = [object](const char* name) -> std::string
    {
        LLNameValue* value = object->getNVPair(name);
        const char* type = value ? value->getType() : nullptr;
        if (!type || std::string(type) != "STRING") return "";
        const char* text = value->getString();
        return text ? text : "";
    };
    return ForMeBadge::parse(field("ForMeBadgeId"), field("ForMeBadgeLabel"), field("ForMeBadgeColor"),
        field("ForMeNameColor"), field("ForMeChatColor"));
}
inline ForMeBadge::Data forChat(const LLChat& chat)
{
    if (chat.mSourceType != CHAT_SOURCE_AGENT || chat.mFromID.isNull() || chat.mMuted || chat.mRlvNamesFiltered
        || !RlvActions::canShowName(RlvActions::SNC_DEFAULT, chat.mFromID)
        || chat.mChatStyle == CHAT_STYLE_HISTORY || chat.mChatStyle == CHAT_STYLE_SERVER_HISTORY) return {};
    return get(chat.mFromID);
}
}
#endif
