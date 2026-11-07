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

#include "formeavatarbadge.h"
#include "formeviewbranding.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    using namespace ForMeBadge;
    for (const auto& role : {std::array<const char*,3>{"owner","Владелец","#6D28D9"},
        {"administrator","Администратор","#D4AF37"}, {"moderator","Модератор","#38BDF8"}, {"designer","Дизайнер","#22C55E"}})
    {
        Data badge = parse(role[0],role[1],role[2],role[2],role[2]);
        assert(badge.valid && badge.badgeColor == badge.chatColor);
        assert(decorate(badge,"Alice") == "[" + badge.label + "] Alice");
        assert(decorate(badge,badge.prefix()+"Alice") == badge.prefix()+"Alice");
    }
    Data custom = parse("event_host","Ведущий","#FaA010","#faa010","#00BBee");
    assert(custom.valid && custom.badgeColor != custom.chatColor);
    assert(parse("event_host","Другой бейдж","#FaA010","#faa010","#00BBee").stamp != custom.stamp);
    assert(!parse("owner","Владелец","#6D28D9","#D4AF37","#6D28D9").valid);
    assert(!parse("","Владелец","#6D28D9","#6D28D9","#6D28D9").valid);
    assert(!parse("Owner","Владелец","#6D28D9","#6D28D9","#6D28D9").valid);
    assert(!parse("owner\nRole","Владелец","#6D28D9","#6D28D9","#6D28D9").valid);
    for (const std::string& label : std::vector<std::string>{"", "<b>Owner</b>", "[Owner]", " Owner", "Owner ", "Owner\nAdmin", "Owner\rAdmin", "Owner&Admin", std::string(33,'a'), "\xC0\xAF", "\xED\xA0\x80", "\xE2\x80\xAE"})
        assert(!parse("owner",label,"#6D28D9","#6D28D9","#6D28D9").valid);
    for (const std::string color : {"", "red", "#123", "#1234567", "#GG1122", "#6D28D9\n", "rgba(1,1,1,1)"})
        assert(!parse("owner","Owner",color,"#6D28D9","#6D28D9").valid);
    assert(eligible(true,true,1,false,false));
    assert(!eligible(false,true,1,false,false)); // scripted object, even with the same five fields
    assert(!eligible(true,false,1,false,false)); // other grid
    assert(!eligible(true,true,2,false,false)); // unsupported server protocol
    assert(!eligible(true,true,1,true,false)); // anonymized identity
    assert(!eligible(true,true,1,false,true)); // replayed log
    assert(decorate(Data(),"Alice") == "Alice"); // missing or revoked role
    assert(decorate(Data(),"[Владелец] Alice") == "[Владелец] Alice"); // text alone is never authority
    assert(ForMeBranding::systemName("Second Life") == "ForMe.World");
    assert(ForMeBranding::systemName("OpenSim") == "ForMe.World");
    assert(ForMeBranding::systemName("FireStorm") == "ForMe.World");
    assert(ForMeBranding::systemName("Alice") == "Alice");
    std::cout << "PASS initial/custom badges, colors, exact-once prefixes, UTF-8/markup rejection, provenance, revocation and system branding\n";
}
