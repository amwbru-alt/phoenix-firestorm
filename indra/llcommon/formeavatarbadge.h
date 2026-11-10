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

#ifndef FORME_AVATAR_BADGE_H
#define FORME_AVATAR_BADGE_H
#include <array>
#include <string>

namespace ForMeBadge
{
struct Data
{
    bool valid = false;
    std::string id;
    std::string label;
    std::string stamp;
    std::array<float, 4> badgeColor {1.f, 1.f, 1.f, 1.f};
    std::array<float, 4> chatColor {1.f, 1.f, 1.f, 1.f};
    std::string prefix() const { return valid ? "[" + label + "] " : ""; }
};
inline bool parseColor(const std::string& value, std::array<float, 4>& result)
{
    if (value.size() != 7 || value.front() != '#') return false;
    std::array<float, 4> parsed {0.f, 0.f, 0.f, 1.f};
    for (unsigned i = 0; i < 3; ++i)
    {
        unsigned channel = 0;
        for (unsigned j = 0; j < 2; ++j)
        {
            unsigned char c = value[1 + i * 2 + j];
            unsigned hex;
            if (c >= '0' && c <= '9') hex = c - '0';
            else if (c >= 'a' && c <= 'f') hex = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') hex = c - 'A' + 10;
            else return false;
            channel = channel * 16 + hex;
        }
        parsed[i] = channel / 255.f;
    }
    result = parsed;
    return true;
}
inline bool validLabel(const std::string& text)
{
    if (text.empty() || text.size() > 128 || text.front() == ' ' || text.back() == ' ') return false;
    unsigned count = 0;
    for (std::size_t i = 0; i < text.size();)
    {
        unsigned char c = text[i++];
        unsigned code = c, extra = 0, minimum = 0;
        if (c >= 0xC2 && c <= 0xDF) { code = c & 31; extra = 1; minimum = 0x80; }
        else if (c >= 0xE0 && c <= 0xEF) { code = c & 15; extra = 2; minimum = 0x800; }
        else if (c >= 0xF0 && c <= 0xF4) { code = c & 7; extra = 3; minimum = 0x10000; }
        else if (c >= 0x80) return false;
        if (i + extra > text.size()) return false;
        for (unsigned j = 0; j < extra; ++j)
        {
            unsigned char next = text[i++];
            if ((next & 0xC0) != 0x80) return false;
            code = (code << 6) | (next & 63);
        }
        if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)
            || code < 32 || (code >= 127 && code <= 159) || (code >= 0x200B && code <= 0x200F)
            || (code >= 0x2028 && code <= 0x202E) || (code >= 0x2060 && code <= 0x206F) || code == 0xFEFF
            || code == '<' || code == '>' || code == '&' || code == '[' || code == ']' || ++count > 32) return false;
    }
    return true;
}
inline Data parse(const std::string& id, const std::string& label, const std::string& badgeColor,
                  const std::string& nameColor, const std::string& chatColor)
{
    Data data;
    if (id.empty() || id.size() > 32 || id.front() < 'a' || id.front() > 'z' || !validLabel(label)) return data;
    for (char c : id) if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_' && c != '-') return data;
    std::array<float, 4> name;
    if (!parseColor(badgeColor, data.badgeColor) || !parseColor(nameColor, name)
        || name != data.badgeColor || !parseColor(chatColor, data.chatColor)) return Data();
    data.valid = true; data.id = id; data.label = label;
    data.stamp = id + "\n" + label + "\n" + badgeColor + "\n" + chatColor;
    return data;
}
inline std::string decorate(const Data& badge, const std::string& name)
{
    if (!badge.valid || name.empty()) return name;
    const std::string prefix = badge.prefix();
    return name.compare(0, prefix.size(), prefix) == 0 ? name : prefix + name;
}
inline bool eligible(bool avatar, bool trustedHost, int protocolVersion, bool filtered, bool history)
{
    return avatar && trustedHost && protocolVersion == 1 && !filtered && !history;
}
}
#endif
