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
"""Check the shipped native map UI and the controls accessed by its C++ code."""
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'indra/newview/skins/default'
source = (ROOT / 'indra/newview/llfloaterworldmap.cpp').read_text()
source = re.sub(r'/\*[\s\S]*?\*/|//[^\n]*', '', source)
textures = {el.get('name'): el.get('file_name') for el in ET.parse(BASE/'textures/textures.xml').getroot()}
expected_buttons = {
    'forme_region_close', 'forme_region_map', 'forme_region_cards',
    'forme_region_zoom_in', 'forme_region_zoom_out',
    'forme_region_detail_tp', 'forme_region_detail_copy',
    *(f'forme_region_details_{i}' for i in range(3))}

for skin in ('default', 'starlight', 'starlightcui'):
    root = ET.parse(ROOT/f'indra/newview/skins/{skin}/xui/en/floater_world_map.xml').getroot()
    names = {el.get('name') for el in root.iter()}
    assert root.get('width') == '746' and root.get('height') == '450'
    assert root.get('can_resize') == 'false'
    assert root.find(".//*[@name='forme_legacy_controls']").get('visible') == 'false'
    def visible_controls(el):
        if el.get('visible') == 'false' and el.get('name') == 'forme_legacy_controls': return set()
        result = {el.get('name')} if el.tag == 'button' else set()
        for child in el: result |= visible_controls(child)
        return result
    assert visible_controls(root) == expected_buttons, (skin, visible_controls(root) ^ expected_buttons)
    # Literal C++ lookups must exist even if retained only as hidden native machinery.
    for control in re.findall(r'getChild(?:View|<[^>]+>)\("([^"\n]+)"\)', source):
        if control == 'hide_beacon_btn': continue  # Separate LLPanelHideBeacon widget.
        assert control in names, (skin, 'missing C++ control', control)
    for i in range(3):
        card = root.find(f".//*[@name='forme_region_card_{i}']")
        assert card.get('width') == '225' and card.get('height') == '305'
        selection = card.find(f".//*[@name='forme_region_selection_{i}']")
        assert int(selection.get('top')) >= 0 and int(selection.get('left')) >= 0, 'negative XUI anchors move the selection outside its card'
        for key in ('photo', 'name', 'weather', 'weather_label', 'visitors', 'selection', 'details'):
            assert card.find(f".//*[@name='forme_region_{key}_{i}']") is not None
    detail = root.find(".//*[@name='forme_region_detail_panel']")
    assert {el.get('name') for el in detail if el.tag == 'text'} == {
        'forme_region_detail_title', 'forme_region_detail_weather_label', 'forme_region_detail_visitors'}
    for el in root.iter():
        for key in ('image_name', 'image_overlay', 'image_unselected', 'image_selected', 'bg_opaque_image', 'bg_alpha_image', 'background_image'):
            asset = el.get(key, '')
            if asset.startswith('Forme_'):
                assert asset in textures and (BASE/'textures'/textures[asset]).is_file(), (skin, asset)

ru = ET.parse(BASE/'xui/ru/floater_world_map.xml').getroot()
assert ru.find("string[@name='forme_region_visitors']").text == '[COUNT] в регионе'
for condition in ('clear', 'cloudy', 'fog', 'rain', 'storm', 'snow'):
    assert ru.find(f"string[@name='forme_weather_{condition}']") is not None
for name, size in {'Window': (746,450), 'Card': (225,305), 'Selected': (233,313), 'Pin': (48,60)}.items():
    data = (BASE/'textures'/textures['Forme_WMap_'+name]).read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n' and struct.unpack('>II', data[16:24]) == size
print('PASS native world-map layout, complete C++ controls, Russian metadata, all skins and assets')
