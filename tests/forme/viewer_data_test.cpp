#include "formeviewerdata.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace ForMeViewerData;
    assert(valid(1,"ForMe.World","Новости мира: добро пожаловать","1.2.3","https://github.com/amwbru-alt/phoenix-firestorm/actions"));
    assert(valid(1,"ForMe.World","","",""));
    assert(!valid(2,"ForMe.World","","",""));
    assert(!valid(1,"other","","",""));
    assert(!valid(1,"ForMe.World",std::string(2049,'a'),"",""));
    assert(!valid(1,"ForMe.World","a\nb","",""));
    assert(!valid(1,"ForMe.World",std::string("\xC0\xAF"),"",""));
    assert(!valid(1,"ForMe.World","Новости","<script>",""));
    for (const char* url:{"https://evil.example/file", "https://github.com/other/project/releases", "https://github.com/amwbru-alt/phoenix-firestorm/actions-evil", "https://github.com/amwbru-alt/phoenix-firestorm/actions/../other", "https://github.com/amwbru-alt/phoenix-firestorm/actions?redirect=evil", "http://185.173.93.200:9000/web/download", "file:///tmp/client", "fmw:///app/login"})
        assert(!allowedDownload(url));
    assert(allowedDownload("http://185.173.93.200:8002/web/downloads/client.zip"));
    assert(allowedDownload("https://github.com/amwbru-alt/phoenix-firestorm/actions/runs/123"));
    std::cout<<"PASS ForMe metadata version, Cyrillic, UTF-8, bounded text and own download destinations\n";
}
