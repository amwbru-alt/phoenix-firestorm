// Copyright (C) 2026, ForMe.World contributors.
// SPDX-License-Identifier: LGPL-2.1-only
#ifndef FORME_STREAM_URL_H
#define FORME_STREAM_URL_H
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace ForMeStream
{
// Region music is an external HTTP stream, independent from grid service URLs.
inline bool allowedURL(std::string_view url)
{
    if (url.empty() || url.size() > 8192) return false;
    for (unsigned char c : url) if (c <= 32 || c == 127 || c == '\\') return false;
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string_view::npos) return false;
    std::string scheme(url.substr(0, scheme_end));
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (scheme != "http" && scheme != "https") return false;
    const auto start = scheme_end + 3;
    const auto end = url.find_first_of("/?#", start);
    const auto authority = url.substr(start, end == std::string_view::npos ? end : end - start);
    if (authority.empty() || authority.find('@') != std::string_view::npos) return false;
    std::string_view host, port;
    if (authority.front() == '[')
    {
        const auto close = authority.find(']');
        if (close == std::string_view::npos || close < 3) return false;
        host = authority.substr(1, close - 1);
        if (host.find(':') == std::string_view::npos) return false;
        for (unsigned char c : host) if (!std::isxdigit(c) && c != ':' && c != '.') return false;
        if (close + 1 < authority.size())
        {
            if (authority[close + 1] != ':') return false;
            port = authority.substr(close + 2);
            if (port.empty()) return false;
        }
    }
    else
    {
        const auto colon = authority.find(':');
        host = authority.substr(0, colon);
        if (host.empty()) return false;
        for (unsigned char c : host) if (!std::isalnum(c) && c != '.' && c != '-' && c != '_') return false;
        if (colon != std::string_view::npos)
        {
            port = authority.substr(colon + 1);
            if (port.empty()) return false;
        }
    }
    if (!port.empty())
    {
        if (port.size() > 5) return false;
        unsigned number = 0;
        for (unsigned char c : port)
        {
            if (c < '0' || c > '9') return false;
            number = number * 10 + c - '0';
        }
        if (number == 0 || number > 65535) return false;
    }
    return true;
}
}
#endif
