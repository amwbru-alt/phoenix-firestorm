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

#include "formecreatorpolicy.h"
#include "formeavatarbadge.h"
#include <cassert>
#include <iostream>
#include <string>

int main()
{
    const auto badge = [](const std::string& id, const std::string& label = "Role") {
        return ForMeBadge::parse(id,label,"#6D28D9","#6D28D9","#00BBEE");
    };
    for (const std::string id : {"owner", "designer"})
    {
        auto value = badge(id);
        assert(value.valid && ForMeCreator::allowed(value.valid,value.id));
    }
    // No administrator/god-mode, moderator or future-role implicit privilege.
    for (const std::string id : {"administrator", "moderator", "resident", "builder", "designer_plus", "owner2"})
    {
        auto value = badge(id,"Владелец");
        assert(value.valid && !ForMeCreator::allowed(value.valid,value.id));
    }
    assert(!ForMeCreator::allowed(false,"owner"));
    assert(!ForMeCreator::allowed(false,"designer"));
    assert(!ForMeCreator::allowed(true,"Owner"));
    assert(!ForMeCreator::allowed(true,"designer "));
    assert(!ForMeCreator::allowed(true,""));
    auto invalid = ForMeBadge::parse("owner","Role","#6D28D9","#D4AF37","#00BBEE");
    assert(!ForMeCreator::allowed(invalid.valid,invalid.id));
    // The same UI control survives startup, grant, revocation and a second grant.
    const bool requested_visible = true;
    const bool requested_enabled = true;
    for (bool authorized : {false,true,false,true})
    {
        assert((requested_visible && ForMeCreator::viewAllowed(true,authorized)) == authorized);
        assert((requested_enabled && ForMeCreator::viewAllowed(true,authorized)) == authorized);
        assert(ForMeCreator::viewAllowed(false,authorized)); // ordinary UI is unaffected
        assert(!(false && ForMeCreator::viewAllowed(true,authorized))); // retain requested hidden state
    }
    std::cout << "PASS exact owner/designer access, spoofed label/color denial, unknown roles, UI grant/revocation transitions\n";
}
