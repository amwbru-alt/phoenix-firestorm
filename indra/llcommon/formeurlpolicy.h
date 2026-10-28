/** ForMe.World region links. Other virtual-world schemes are never accepted. */
#ifndef FORME_URL_POLICY_H
#define FORME_URL_POLICY_H
#include "formegridpolicy.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
namespace ForMeURL
{
struct Link
{
    bool valid = false, app = false;
    std::string region, query;
    std::vector<std::string> path;
    std::array<float, 3> position{128.f,128.f,25.f};
};
inline int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
inline bool decode(const std::string& input, std::string& output)
{
    output.clear();
    for (std::size_t i=0;i<input.size();++i)
    {
        unsigned char c = input[i];
        if (c == '%')
        {
            if (i+2 >= input.size() || hex(input[i+1]) < 0 || hex(input[i+2]) < 0) return false;
            c = static_cast<unsigned char>(hex(input[i+1])*16+hex(input[i+2])); i+=2;
        }
        if (c < 32 || c == 127 || c == '/' || c == '\\') return false;
        output.push_back(static_cast<char>(c));
    }
    // Strict UTF-8, excluding overlong forms, surrogate codepoints and controls.
    for (std::size_t i=0;i<output.size();)
    {
        std::uint32_t value=static_cast<unsigned char>(output[i++]); unsigned extra=0;
        if (value < 128) continue;
        if (value >= 0xC2 && value <= 0xDF) { extra=1; value&=31; }
        else if (value >= 0xE0 && value <= 0xEF) { extra=2; value&=15; }
        else if (value >= 0xF0 && value <= 0xF4) { extra=3; value&=7; }
        else return false;
        if (i+extra > output.size()) return false;
        for (unsigned j=0;j<extra;++j)
        {
            const unsigned char c=output[i++]; if ((c&0xC0)!=0x80) return false;
            value=(value<<6)|(c&63);
        }
        if ((extra==1 && value<128) || (extra==2 && value<2048) || (extra==3 && value<65536)
            || (value>=0xD800 && value<=0xDFFF) || value>0x10FFFF || (value>=0x80 && value<=0x9F)
            || (value>=0x202A && value<=0x202E) || (value>=0x2066 && value<=0x2069)) return false;
    }
    return true;
}
inline Link parse(const std::string& url)
{
    Link result;
    if (!url.starts_with("fmw://") || url.size()>4096 || url.find('#')!=std::string::npos) return result;
    const auto question=url.find('?');
    const std::string body=url.substr(6,question==std::string::npos?std::string::npos:question-6);
    const auto slash=body.find('/');
    const std::string authority=body.substr(0,slash);
    std::vector<std::string> parts;
    std::string rest=slash==std::string::npos?"":body.substr(slash+1);
    while (!rest.empty())
    {
        const auto end=rest.find('/'); const auto part=rest.substr(0,end);
        if (part.empty()) return result;
        std::string decoded; if (!decode(part,decoded)) return result;
        parts.push_back(decoded);
        if (end==std::string::npos) break;
        rest=rest.substr(end+1);
    }
    const bool grid=authority==ForMeGrid::GridID;
    if ((authority.empty() || grid) && !parts.empty() && parts.front()=="app")
    {
        parts.erase(parts.begin()); if (parts.empty()) return result;
        result.valid=true;result.app=true;result.path=parts;
        if (question!=std::string::npos) result.query=url.substr(question+1);
        return result;
    }
    if (question!=std::string::npos) return result;
    if (!grid)
    {
        std::string name;
        if (authority.empty() || !decode(authority,name)) return result;
        parts.insert(parts.begin(),name);
    }
    if (parts.empty() || parts.size()>4 || !ForMeGrid::localRegion(parts.front()) || parts.front().size()>255) return result;
    result.region=parts.front();
    for (std::size_t i=1;i<parts.size();++i)
    {
        float number;
        std::istringstream stream(parts[i]);stream.imbue(std::locale::classic());stream>>std::noskipws>>number;
        if (!stream || !stream.eof() || !std::isfinite(number) || number>65536.f || number<(i==3?-4096.f:0.f)) return Link{};
        result.position[i-1]=number;
    }
    result.valid=true;return result;
}
inline bool blockedWeb(const std::string& url)
{
    const auto split=url.find("://");if(split==std::string::npos)return false;
    std::string host=url.substr(split+3);host=host.substr(0,host.find_first_of("/?#"));
    const auto at=host.rfind('@');if(at!=std::string::npos)host.erase(0,at+1);
    host=host.substr(0,host.find(':'));
    std::string decodedHost;
    if (decode(host, decodedHost)) host = decodedHost;
    for(char& c:host)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
    while(!host.empty()&&host.back()=='.')host.pop_back();
    for(const std::string domain:{"secondlife.com","secondlife.io","secondlifegrid.net","lindenlab.com","slurl.com","firestormviewer.org","firestormviewer.com","phoenixviewer.com","phoenixviewer.org","secondlife-staging.com"})
        if(host==domain || host.ends_with("."+domain))return true;
    return false;
}
}
#endif
