#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Check real window navigation, retained native controls and line-icon geometry."""
from pathlib import Path
import json
import re
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'indra/newview/skins/default'
XUI = BASE / 'xui/en'

def resource(name):
    return ET.parse(XUI / name).getroot()

def child(root, name):
    result = root.find(".//*[@name='%s']" % name)
    assert result is not None, name
    return result

textures = {x.get('name'): x for x in ET.parse(BASE / 'textures/textures.xml').getroot()}
aliases = json.loads((BASE / 'textures/forme_ui/aliases.json').read_text())
for name, (filename, role, width, height, clip) in aliases.items():
    assert textures[name].get('file_name') == filename, name
    image = (BASE / 'textures' / filename).read_bytes()
    assert image[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II', image[16:24]) == (1024, 1024), name
    assert clip['right'] - clip['left'] == width and clip['top'] - clip['bottom'] == height
    for key, value in clip.items():
        assert int(textures[name].get('clip.' + key)) == value
        assert 0 <= value <= 1024
# Preserve the information carried by native item/permission states.
for first, second in [('Inv_Object', 'Inv_Object_Multi'),
                      ('Inv_CallingCard', 'Inv_CallingCardOffline'),
                      ('Inv_Favorite_Star_Content', 'Inv_Favorite_Star_Full'),
                      ('Profile_Perm_Online_Enabled', 'Profile_Perm_Online_Disabled'),
                      ('Profile_Perm_Find_Enabled', 'Profile_Perm_Find_Disabled'),
                      ('Profile_Perm_Objects_Enabled', 'Profile_Perm_Objects_Disabled')]:
    assert aliases[first][1:] != aliases[second][1:]
# Shipped skins and themes must not restore old filled icons or square controls.
for path in (ROOT / 'indra/newview/skins').glob('**/textures/textures.xml'):
    if path == BASE / 'textures/textures.xml':
        continue
    for node in ET.parse(path).getroot():
        name = node.get('name')
        if name in aliases:
            assert node.attrib == textures[name].attrib, (path, name)

registered = re.findall(r'LLFloaterReg::add\(\s*"([^"]+)"\s*,\s*"([^"]+)"',
                        (ROOT / 'indra/newview/llviewerfloaterreg.cpp').read_text())
custom = {'add_payment_method', 'bulk_upload', 'fs_camera_small', 'fs_nearby_chat',
          'emoji_complete', 'mini_map', 'new_feature_notification', 'world_map', 'avatar_align_mini'}
framed = 0
for identifier, filename in registered:
    root = resource(filename)
    if identifier in custom:
        continue
    assert int(root.get('header_height', '0')) == 44, filename
    assert root.get('header_icon') in textures, filename
    framed += 1

inventory = resource('panel_main_inventory.xml')
filters = {node.get('value') for node in child(inventory, 'filter_combo_box')}
buttons = child(inventory, 'forme_inventory_categories_body').findall('button')
assert len(buttons) == 15
for button in buttons:
    cb = button.find('button.commit_callback')
    assert cb.get('function') == 'ForMeInventory.Category'
    assert cb.get('parameter') in filters
    assert button.find('button.is_toggled_callback').get('function') == 'ForMeInventory.CategorySelected'
people = resource('panel_people.xml')
native_tabs = {x.get('name') for x in child(people, 'tabs').findall('panel')}
nav = child(people, 'forme_people_navigation').findall('button')
assert len(nav) == 6
assert {x.find('button.commit_callback').get('parameter') for x in nav} == native_tabs
assert child(people, 'tabs').get('hide_tabs') == 'true'
# Every tile stays inside the 442px pane and is independently reachable.
for first in nav:
    x, y = int(first.get('left')), int(first.get('top'))
    w, h = int(first.get('width')), int(first.get('height'))
    assert 0 <= x and x + w <= 442 and 0 <= y and y + h <= 80
    for second in nav:
        if first is second:
            continue
        sx, sy = int(second.get('left')), int(second.get('top'))
        sw, sh = int(second.get('width')), int(second.get('height'))
        assert x + w <= sx or sx + sw <= x or y + h <= sy or sy + sh <= y

item = resource('sidepanel_item_info.xml')
assert child(item, 'CheckOwnerModify').get('left') != child(item, 'CheckNextOwnerModify').get('left')
assert child(item, 'item_scroll_container').find("./layout_stack[@name='main_stack']") is not None
move = resource('floater_moveview.xml')
for name in ['forward btn', 'backward btn', 'move left btn', 'move right btn']:
    assert child(move, name).tag in ['joystick_turn', 'joystick_slide'], name
assert child(move, 'panel_modes') is not None
assert int(move.get('min_height')) >= int(move.get('height')) + 44
camera = resource('floater_camera.xml')
assert int(camera.get('min_height')) >= int(camera.get('height')) + 44
assert child(camera, 'cam_rotate_stick').tag == 'joystick_rotate'
assert child(camera, 'cam_track_stick').tag == 'joystick_track'
assert int(child(camera, 'zoom').get('left')) < int(child(camera, 'preset_buttons_panel').get('left'))
# Fail if a new icon binding is missing in any modified native resource.
for path in XUI.rglob('*.xml'):
    root = ET.parse(path).getroot()
    for node in root.iter():
        for key, value in node.attrib.items():
            if value.startswith('Forme_UI_'):
                assert value in textures, (path.name, node.get('name'), key, value)
print(f'PASS {framed} registered frames, {len(aliases)} line-icon aliases, native category/tab navigation and retained joystick classes')
