// Copyright (C) 2026, ForMe.World contributors.
// SPDX-License-Identifier: LGPL-2.1-only
#ifndef FORME_RETIRED_SETTINGS_H
#define FORME_RETIRED_SETTINGS_H
#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace ForMeSettings
{
inline std::string replacement(std::string_view name)
{
    if (name == "VoiceCallsFriendsOnly") return "IMFriendsOnly";
    if (name == "SpeakerParticipantDefaultOrder") return "ChatParticipantSortOrder";
    return {};
}
inline bool retired(std::string_view input)
{
    std::string name(input);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    // Payment invoice names are unrelated to the removed communication feature.
    for (auto position = name.find("invoice"); position != std::string::npos; position = name.find("invoice")) name.erase(position, 7);
    for (std::string_view marker : {"voice", "vivox", "webrtc", "microphone", "pushtotalk"})
        if (name.find(marker) != std::string::npos) return true;
    constexpr std::array<std::string_view, 13> other = {
        "firstspeak", "audiolevelmic", "autodisengagemic", "lipsyncaah", "lipsyncaahpowertransfer",
        "lipsyncenabled", "lipsyncooh", "lipsyncoohaahrate", "lipsyncoohpowertransfer",
        "pttcurrentlyenabled", "uisndmictoggle", "playmodeuisndmictoggle",
        "speakerparticipantdefaultorder"};
    return std::find(other.begin(), other.end(), std::string_view(name)) != other.end();
}
}
#endif
