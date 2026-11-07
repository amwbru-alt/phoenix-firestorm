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

#include "formegridpolicy.h"
#include <cassert>
#include <iostream>
#include <string>

int main()
{
    using namespace ForMeGrid;
    assert(allowedLoginURI(LoginURI));
    assert(allowedLoginURI("http://185.173.93.200:8002"));
    for (const auto& uri : {"", "https://login.agni.lindenlab.com/cgi-bin/login.cgi", "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
         "http://other.example:8002/", "http://185.173.93.200/", "https://185.173.93.200:8002/", "http://185.173.93.200:8003/",
         "http://185.173.93.200:8002.evil/", "http://185.173.93.200:8002@evil/", "http://evil@185.173.93.200:8002/",
         "http://185.173.93.200:8002/path", "http://185.173.93.200:8002/?next=evil", "http://185.173.93.200:8002/#evil",
         "http://185.173.93.200:08002/", "//185.173.93.200:8002/", "http://185.173.93.200:8002/../", "http://185.173.93.200:8002/\n"})
        assert(!allowedLoginURI(uri));
    assert(allowedGrid(GridID) && allowedGrid(Label) && allowedGrid("forme"));
    assert(!allowedGrid("agni") && !allowedGrid("Second Life") && !allowedGrid("localhost:9000"));
    assert(allowedCredential("First Last@185.173.93.200:8002"));
    assert(!allowedCredential("First Last@util.agni.lindenlab.com") && !allowedCredential("First Last") && !allowedCredential("@185.173.93.200:8002"));
    assert(allowedSimulator("185.173.93.200") && !allowedSimulator("127.0.0.1") && !allowedSimulator("185.173.93.201"));
    assert(localRegion("My Region") && !localRegion("other.example:8002:Region") && !localRegion("http://other/Region"));
    for (const auto& uri : {"http://185.173.93.200:9000/CAPS/seed", "https://185.173.93.200:8003/CAPS/id", "http://185.173.93.200/CAPS/id"})
        assert(allowedServiceURI(uri));
    for (const auto& uri : {"", "file:///etc/passwd", "http://evil/CAPS/seed", "http://185.173.93.200.evil:9000/CAPS/seed",
         "http://185.173.93.200@evil/CAPS/seed", "http://evil@185.173.93.200/CAPS/seed", "http://185.173.93.200:0/CAPS/seed",
         "http://185.173.93.200:65536/CAPS/seed", "http://185.173.93.200:9x/CAPS/seed", "http://185.173.93.200:9000\\@evil/CAPS/seed",
         "http://185.173.93.200:9000/CAPS/seed#evil", "http://185.173.93.200:9000/\r\nHost: evil"})
        assert(!allowedServiceURI(uri));
    // Appending bytes to the one permitted endpoint must not turn it into a prefix allowlist.
    const std::string suffixes = "abcdefghijklmnopqrstuvwxyz0123456789/@?#:%\\\r\n ";
    for (char c : suffixes) assert(!allowedLoginURI(std::string(LoginURI) + c));
    std::cout << "PASS fixed login, SL/foreign-grid rejection, credentials, simulator/seed policy and URI bypass cases\n";
}
