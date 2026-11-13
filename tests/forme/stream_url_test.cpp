// Copyright (C) 2026, ForMe.World contributors.
// SPDX-License-Identifier: LGPL-2.1-only
#include "formestreamurl.h"
#include "formeretiredsettings.h"
#include <cassert>
int main()
{
    for (auto key : {"EnableVoiceChat", "MuteVoice", "VoiceCallsFriendsOnly", "UnknownVivoxOption", "WebRTCSession", "AudioLevelMic", "PushToTalkToggle", "LipSyncEnabled", "PTTCurrentlyEnabled", "InvoiceVoiceDebug"}) assert(ForMeSettings::retired(key));
    for (auto key : {"AudioStreamingMusic", "AudioLevelMusic", "MuteAudio", "IMFriendsOnly", "SpeakerParticipantRemoveDelay", "ShowVolumeSettingsPopup", "ChatParticipantSortOrder", "InvoiceUpdates", "DynamicLOD"}) assert(!ForMeSettings::retired(key));
    assert(ForMeSettings::replacement("VoiceCallsFriendsOnly") == "IMFriendsOnly");
    assert(ForMeSettings::replacement("SpeakerParticipantDefaultOrder") == "ChatParticipantSortOrder");
    assert(ForMeSettings::replacement("EnableVoiceChat").empty());
    for (auto url : {"https://radio.example.org/live.mp3", "http://icecast.example.org:8000/live", "HTTP://radio.example.org/live.ogg?token=a%20b", "http://127.0.0.1:8080/music", "https://[2001:db8::1]:8443/stream", "http://forme.world:8002/live", "https://music.example.org/live?session=abc&format=mp3"}) assert(ForMeStream::allowedURL(url));
    for (auto url : {"", "file:///etc/passwd", "javascript:alert(1)", "fmw://region", "https:///missing-host", "https://user:password@radio.example.org/live", "http://radio.example.org:0/live", "http://radio.example.org:65536/live", "http://radio.example.org:/live", "http://radio.example.org:abc/live", "http://[::1/live", "http://radio.example.org/stream\nnext", "http://radio.example.org\\stream", "http://radio.example.org:8000:9000/live"}) assert(!ForMeStream::allowedURL(url));
}
