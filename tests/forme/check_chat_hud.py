#!/usr/bin/env python3
#  * @file
#  * @brief
#  *
#  * $LicenseInfo:firstyear=2026&license=viewerlgpl$
#  * Phoenix Firestorm Viewer Source Code
#  * Copyright (C) 2026, ForMe.World contributors.
#  *
#  * This library is free software; you can redistribute it and/or
#  * modify it under the terms of the GNU Lesser General Public
#  * License as published by the Free Software Foundation;
#  * version 2.1 of the License only.
#  *
#  * This library is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  * Lesser General Public License for more details.
#  *
#  * You should have received a copy of the GNU Lesser General Public
#  * License along with this library; if not, write to the Free Software
#  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#  *
#  * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
#  * http://www.firestormviewer.org
#  * $/LicenseInfo$
#
"""Validate all native chat lookups, dock commands and packaged HUD assets."""
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'indra/newview/skins/default'
textures = {el.get('name'): el.get('file_name') for el in ET.parse(BASE/'textures/textures.xml').getroot()}
source = (ROOT/'indra/newview/fsfloaternearbychat.cpp').read_text()
source = re.sub(r'/\*[\s\S]*?\*/|//[^\n]*', '', source)
for skin in ('default', 'starlight', 'starlightcui', 'vintage'):
    root = ET.parse(ROOT/f'indra/newview/skins/{skin}/xui/en/floater_fs_nearby_chat.xml').getroot()
    names = [el.get('name') for el in root.iter() if el.tag not in ('floater.string', 'panel.string') and el.get('name')]
    assert len(names) == len(set(names)), (skin, 'duplicate native chat control')
    for name in re.findall(r'getChild(?:View|<[^>]+>)\("([^"\n]+)"\)', source):
        assert name in names, (skin, 'missing native chat control', name)
    assert root.get('width') == '380' and root.get('height') == '204'
    assert root.get('save_rect') == 'false' and root.get('can_close') == 'false'
    assert root.find(".//*[@name='forme_chat_legacy']").get('visible') == 'false'
    assert root.find(".//*[@name='chat_history']").get('message_header') == 'panel_forme_chat_header.xml'
    assert root.find(".//*[@name='chat_box']").tag == 'chat_editor'
    assert root.find(".//*[@name='send_chat']").get('image_overlay') == 'Forme_HUD_Send'
    for el in root.iter():
        for key in ('image_name', 'image_overlay', 'bg_opaque_image', 'image_unselected'):
            name = el.get(key, '')
            if name.startswith('Forme_'):
                assert name in textures and (BASE/'textures'/textures[name]).is_file(), (skin, name)
for skin in ('default', 'starlight', 'starlightcui'):
    root = ET.parse(ROOT/f'indra/newview/skins/{skin}/xui/en/panel_toolbar_view.xml').getroot()
    dock = root.find(".//*[@name='forme_compact_dock']")
    buttons = dock.findall('.//button')
    assert len(buttons) == 4
    actions = [(b.find('commit_callback').get('function'), b.find('commit_callback').get('parameter')) for b in buttons]
    assert actions == [('Floater.Toggle','world_map'), ('Floater.Toggle','people'), ('Floater.Toggle','inventory'), ('Avatar.OpenMarketplace',None)]
    assert all(el.get('image_name','').startswith('Forme_HUD_') for el in dock.findall('.//icon'))
for path in (ROOT/'indra/newview/skins').glob('*/xui/*/floater_fs_nearby_chat.xml'):
    if path.parent.name == 'en': continue
    locale = ET.parse(path).getroot()
    assert locale.find(".//*[@name='ls_chat']") is None, (path, 'stale translated layout would restore old chat')
for name in ('Surface','Dock','Active','Input'):
    path = BASE/'textures'/textures['Forme_HUD_'+name]
    assert struct.unpack('>II', path.read_bytes()[16:24]) == (64,64)
header = ET.parse(BASE/'xui/en/panel_forme_chat_header.xml').getroot()
assert {'avatar_icon','user_name','time_box','header_ls'} <= {el.get('name') for el in header.iter()}
print('PASS native chat controls, real history/editor, all skins, four dock commands and packaged line icons')
