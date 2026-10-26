/** Public ForMe.World metadata. No role, settings or login-policy fields. */
#ifndef FORME_VIEWER_DATA_H
#define FORME_VIEWER_DATA_H
#include <string>
#include <cstdint>
namespace ForMeViewerData
{
inline constexpr const char* Endpoint = "http://185.173.93.200:8002/api/web/viewer/data";
inline bool safeText(const std::string& text, std::size_t limit)
{
    if (text.size() > limit) return false;
    for (std::size_t i=0;i<text.size();)
    {
        std::uint32_t value=static_cast<unsigned char>(text[i++]); unsigned extra=0;
        if (value < 32 || value == 127) return false;
        if (value < 128) continue;
        if (value>=0xC2 && value<=0xDF) { extra=1;value&=31; }
        else if (value>=0xE0 && value<=0xEF) { extra=2;value&=15; }
        else if (value>=0xF0 && value<=0xF4) { extra=3;value&=7; }
        else return false;
        if (i+extra>text.size()) return false;
        for (unsigned j=0;j<extra;++j) { unsigned char c=text[i++];if((c&0xC0)!=0x80)return false;value=(value<<6)|(c&63); }
        if ((extra==1&&value<128)||(extra==2&&value<2048)||(extra==3&&value<65536)
            ||(value>=0xD800&&value<=0xDFFF)||value>0x10FFFF||(value>=0x80&&value<=0x9F)
            ||(value>=0x202A&&value<=0x202E)||(value>=0x2066&&value<=0x2069))return false;
    }
    return true;
}
inline bool allowedDownload(const std::string& url)
{
    if (url.empty()) return true;
    if (!safeText(url,1024) || url.find_first_of(" \\%?#")!=std::string::npos || url.find("..")!=std::string::npos) return false;
    if (url.starts_with("http://185.173.93.200:8002/web/") || url.starts_with("https://185.173.93.200:8002/web/")) return true;
    const std::string root="https://github.com/amwbru-alt/phoenix-firestorm/";
    for (const char* section:{"actions","releases"})
    {
        const std::string prefix=root+section;
        if (url==prefix || url.starts_with(prefix+"/")) return true;
    }
    return false;
}
inline bool valid(int version, const std::string& service, const std::string& motd,
    const std::string& latest, const std::string& download)
{
    if (version!=1 || service!="ForMe.World" || !safeText(motd,2048) || !safeText(latest,64) || !allowedDownload(download)) return false;
    for (unsigned char c:latest) if (!((c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='.'||c=='-'))return false;
    return true;
}
}
#endif
