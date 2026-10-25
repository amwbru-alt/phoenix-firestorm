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
