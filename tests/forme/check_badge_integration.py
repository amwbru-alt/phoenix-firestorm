#  * @file
#  * @brief
#  *
#  * $LicenseInfo:firstyear=2026&license=viewerlgpl$
#  * Phoenix Firestorm Viewer Source Code
#  * Copyright (C) 2026, ForMe.World contributors.
#  *
#  * This library is free software; you can redistribute it and/or
#  * modify it under the terms of the GNU Lesser General Public
#  * License as published by the Free Software Foundation;
#  * version 2.1 of the License only.
#  *
#  * This library is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  * Lesser General Public License for more details.
#  *
#  * You should have received a copy of the GNU Lesser General Public
#  * License along with this library; if not, write to the Free Software
#  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#  *
#  * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
#  * http://www.firestormviewer.org
#  * $/LicenseInfo$
#
"""Structural integration checks; these do not replace compiling/running the viewer."""
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
def read(path): return (ROOT/path).read_text(encoding='utf-8')
def contains(path,*needles):
    content=read(path)
    for needle in needles:
        assert needle in content, (path,needle)
contains('indra/newview/fsavatarbadges.h','object->isAvatar()', 'allowedSimulator(region->getHost().getIPString())',
         'advertised["ForMeAvatarBadges"]["version"]','ForMeBadgeId','ForMeBadgeLabel','ForMeBadgeColor','ForMeNameColor',
         'ForMeChatColor','CHAT_SOURCE_AGENT','chat.mMuted','chat.mRlvNamesFiltered','CHAT_STYLE_HISTORY',
         'CHAT_STYLE_SERVER_HISTORY','value->getType()', '"STRING"')
contains('indra/newview/llviewerobject.cpp','LLVOAvatar::invalidateNameTag(getID())', 'forme_previous_badge != ForMeAvatarBadges::get(getID()).stamp')
contains('indra/newview/llviewerregion.cpp','LLVOAvatar::invalidateNameTags()')
contains('indra/newview/llvoavatar.cpp','ForMeBadge::decorate(forme_badge, line)','is_name && (isSelf() || RlvActions::canShowName',
         'ForMeAvatarBadges::color(forme_badge.chatColor)','ForMeAvatarBadges::color(forme_badge.badgeColor)')
contains('indra/newview/llviewerchat.cpp','ForMeAvatarBadges::forChat(chat)','ForMeAvatarBadges::color(forme_badge.chatColor)')
for history in ('fschathistory','llchathistory'):
    contains(f'indra/newview/{history}.cpp','mForMeBadgeEligible','RlvActions::canShowName',
             'ForMeBadge::decorate(forme_badge, chat.mFromName)','ForMeBadge::decorate(forme_badge, av_name.getDisplayName())',
             'mForMeLastBadge == forme_badge.stamp','mForMeLastBadge = forme_badge.stamp','mForMeLastBadge.clear()',
             'ForMeBranding::legacyName(chat.mFromName)')
    contains(f'indra/newview/{history}.h','mForMeLastBadge')
console=read('indra/newview/fsconsoleutils.cpp').split('void FSConsoleUtils::onProcessChatAvatarNameLookup',1)[1]
assert 'ForMeBadge::decorate(badge, sender_name)' in console
contains('indra/llui/llconsole.cpp', 'mParagraphColorSegments.clear()', 'paragraph.mBodyColor', 'mColoredPrefix', 'mLinePrefixes.pop_front()')
contains('indra/llui/llconsole.h', 'mBodyColor', 'mPrefixColor', 'mColoredPrefix', 'mSourcePrefix', 'mLinePrefixes')
# No viewer-side static role roster: arbitrary valid server roles are supported.
assert 'Владелец' not in read('indra/newview/fsavatarbadges.h')
for path in (ROOT/'indra/newview/skins').glob('*/xui/*/*.xml'):
    ET.parse(path)
for path in ('indra/newview/Firestorm.xib','indra/newview/SecondLife.xib'):
    ET.parse(ROOT/path)
print('PASS badge integration boundaries, nametags, both histories, console callback and all XUI XML')
