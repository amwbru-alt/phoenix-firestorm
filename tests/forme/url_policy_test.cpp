#include "formeurlpolicy.h"
#include <cassert>
#include <iostream>
int main()
{
    const auto link=ForMeURL::parse("fmw://185.173.93.200:8002/%D0%9C%D0%B8%D1%80/128/64/25");
    assert(link.valid && !link.app && link.region=="Мир" && link.position[1]==64);
    assert(ForMeURL::parse("fmw://Русский регион/256/128/40").valid);
    assert(ForMeURL::parse("fmw:///app/agent/11111111-1111-1111-1111-111111111111/inspect").app);
    for(const char* url:{"hop://185.173.93.200:8002/Мир/1/2/3","secondlife://Мир/1/2/3","https://maps.secondlife.com/secondlife/Мир/1/2/3", "fmw://evil.example:8002/Мир/1/2/3", "fmw://user@Мир/1/2/3", "fmw://Мир/nan/2/3", "fmw://Мир/1/2/3/4", "fmw://Мир/1/2/3?grid=evil", "fmw://%C0%AF/1/2/3", "fmw://%00/1/2/3", "fmw://Мир/-1/2/3"})assert(!ForMeURL::parse(url).valid);
    assert(ForMeURL::blockedWeb("https://wiki.firestormviewer.org/wiki"));
    assert(ForMeURL::blockedWeb("https://SECONDlife.com./account"));
    assert(ForMeURL::blockedWeb("https://www.firestormviewer.org%2e/path"));
    assert(!ForMeURL::blockedWeb("http://185.173.93.200:8002/web/marketplace.html"));
    assert(!ForMeURL::blockedWeb("https://secondlife.com.example.org/"));
    std::cout<<"PASS fmw-only links, Cyrillic UTF-8, coordinates, grid boundaries and upstream website blocks\n";
}
