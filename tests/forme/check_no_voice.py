#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Protect removal of the communication stack and the region music exception."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
NV = ROOT / 'indra/newview'
for stem in ('llvoiceclient', 'llvoicechannel', 'llvoicewebrtc', 'llvoicevisualizer',
             'llvoicecallhandler', 'llpanelvoicedevicesettings', 'llfloatervoicevolume',
             'llfloaterchatvoicevolume', 'fsfloatervoicecontrols', 'fsnearbychatvoicemonitor',
             'llnearbyvoicemoderation', 'llspeakingindicatormanager', 'lloutputmonitorctrl'):
    assert not any((NV / (stem + extension)).exists() for extension in ('.cpp', '.h')), stem
assert not any((ROOT / 'indra/llwebrtc').glob('*.cpp'))
packages = ET.parse(ROOT / 'autobuild.xml')
assert not {'webrtc', 'slvoice'} & {node.text for node in packages.iter('key')}
for path in (NV / 'Info-Firestorm.plist', NV / 'Info-SecondLife.plist', NV / 'slplugin.entitlements'):
    assert 'NSMicrophoneUsageDescription' not in path.read_text(), path
    assert 'com.apple.security.device.audio-input' not in path.read_text(), path
for path in (NV / 'viewer_manifest.py', NV / 'fs_viewer_manifest.py'):
    active = '\n'.join(line for line in path.read_text().splitlines() if not line.lstrip().startswith('#'))
    assert not re.search(r'webrtc|slvoice|vivox', active, re.I), path
assert 'ForMeSettings::retired(name)' in (ROOT / 'indra/llxml/llcontrol.cpp').read_text()
assert 'ForMeSettings::retired(xml_entry.control_name.getValue())' in (NV / 'quickprefs.cpp').read_text()
for path in (NV / 'installers/windows').glob('*.nsi'):
    data = path.read_bytes()
    source = data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8', errors='replace')
    assert not re.search(r'vivox|slvoice|webrtc', source, re.I), path
avatar = (NV / 'llvoavatar.cpp').read_text() + (NV / 'llvoavatar.h').read_text()
assert not re.search(r'lip.?sync|mOohMorph|mAahMorph', avatar, re.I)
assert 'Lipsync_' not in (NV / 'character/avatar_lad.xml').read_text()
for path in NV.glob('*.cpp'):
    assert 'flagReservedLegacy' not in path.read_text(errors='replace'), path
assert 'body["allow_voice_chat"] = false;' in (NV / 'llestateinfomodel.cpp').read_text()
for path in (ROOT / 'indra').rglob('*'):
    if path.suffix not in ('.cpp', '.h', '.cmake') and path.name != 'CMakeLists.txt':
        continue
    source = path.read_text(errors='replace')
    code = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    assert not re.search(r'\b(?:LLVoice\w*|LLWebRTC\w*|WEBRTC_\w*|SLVoice)\b', code), path
    assert not re.search(r'\bllwebrtc\b|use_prebuilt_binary\s*\(\s*slvoice', code, re.I), path

for name in ('settings.xml', 'settings_per_account.xml', 'settings_text.xml', 'cmd_line.xml', 'message.xml'):
    root = ET.parse(NV / 'app_settings' / name).getroot()
    assert not any(re.search(r'voice|vivox|webrtc|microphone', node.text or '', re.I)
                   for node in root.iter('key')), name
music_settings = ET.parse(NV / 'app_settings/settings.xml')
keys = {node.text for node in music_settings.iter('key')}
assert {'AudioStreamingMusic', 'AudioLevelMusic', 'MuteMusic', 'AudioLevelMaster', 'MuteAudio'} <= keys
for path in (NV / 'skins').rglob('*.xml'):
    root = ET.parse(path).getroot()
    for node in root.iter():
        for key, value in node.attrib.items():
            assert not re.search(r'voice|vivox|webrtc|microphone|push_to_talk|SpeakBtn|start_call_button_tooltip|end_call_button_tooltip|group_call_btn|call_btn_lp', key + '=' + value, re.I), (path, key, value)
        assert not re.search(r'voice|output_monitor|chiclet_speaker|talk_button|speak_button', node.tag, re.I), (path, node.tag)
for path in (NV / 'skins').rglob('*'):
    if path.is_file():
        assert not re.search(r'voice|vivox|webrtc|microphone|SpeakBtn|^call_(?:end_)?icon_24x24|^talk_button\.xml$', path.name, re.I), path

# The original music backend, player lifecycle, volume and media plugin routing remain.
backend = (NV / 'llviewermedia_streamingaudio.cpp').read_text()
for operation in ('initializeMedia("audio/mpeg")', 'loadURI(snt_url)', 'mMediaPlugin->start()',
                  'mMediaPlugin->pause()', 'mMediaPlugin->stop()', 'mMediaPlugin->setVolume(vol)'):
    assert operation in backend, operation
audio = (NV / 'llvieweraudio.cpp').read_text()
assert 'ForMeStream::allowedURL(streamURI)' in audio
assert 'allowedServiceURI(streamURI)' not in audio
assert 'gAudiop->setInternetStreamGain' in audio
land = (NV / 'llpanellandaudio.cpp').read_text()
assert 'parcel->setMusicURL(music_url)' in land and 'mMusicURLEdit' in land
assert 'mParcelStreamPlayButton' in (NV / 'utilitybar.cpp').read_text()
assert 'getParcelFlagAllowVoice' not in land
for name in ('llspeakers.h', 'llspeakers.cpp'):
    assert 'LLVoiceChannel' not in (NV / name).read_text()
assert 'data["method"] = "decline invitation";' in (NV / 'llimview.cpp').read_text()
assert 'new LLIMSpeakerMgr(mSessionID)' in (NV / 'llimview.cpp').read_text()
print('PASS no communication providers, calls, settings or controls; text sessions and region HTTP music retained')
