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
    assert int(root.get('header_height', '0')) == (28 if filename in {'floater_moveview.xml', 'floater_camera.xml'} else 44), filename
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
# The default grid fits before resize, then the People panel balances all three columns.
for first in nav:
    x, y = int(first.get('left')), int(first.get('top'))
    w, h = int(first.get('width')), int(first.get('height'))
    assert 0 <= x and x + w <= 442 and 0 <= y and y + h <= 64
    for second in nav:
        if first is second:
            continue
        sx, sy = int(second.get('left')), int(second.get('top'))
        sw, sh = int(second.get('width')), int(second.get('height'))
        assert x + w <= sx or sx + sw <= x or y + h <= sy or sy + sh <= y

# ForMe social and commerce windows keep their expanded, balanced geometry.
people_floater = resource('floater_people.xml')
assert int(people_floater.get('height')) == 580
assert int(child(people_floater, 'main_panel').get('height')) == 552
assert int(people.get('height')) == 552
market_listings = resource('floater_marketplace_listings.xml')
assert int(market_listings.get('width')) == 440
assert int(market_listings.get('height')) == 560
assert int(child(market_listings, 'marketplace_listings_panel').get('width')) == 440
group_picker = resource('floater_choose_group.xml')
assert int(group_picker.get('width')) == 360
assert int(child(group_picker, 'group list').get('width')) == 328
profile_permissions = resource('floater_profile_permissions.xml')
assert profile_permissions.get('bg_opaque_image') == 'Forme_Round_Window'
assert all(int(child(profile_permissions, name).get('width')) == 340
           for name in ('online_check', 'map_check', 'objects_check'))
experience_picker = resource('floater_experience_search.xml')
assert int(experience_picker.get('width')) >= int(experience_picker.get('min_width'))
group_titles = resource('floater_fs_group_titles.xml')
assert int(group_titles.get('width')) == 600
assert int(group_titles.get('height')) == 420
assert int(child(group_titles, 'title_list').get('height')) == 330
# Dialog controls use explicit anchors and balanced action rows, not only larger shells.
assert child(group_titles, 'filter_input').get('left') == '16'
assert child(group_titles, 'filter_input').get('right') == '-16'
assert child(group_titles, 'title_list').get('top') == '52'
assert child(group_titles, 'title_list').get('bottom') == '-88'
for name in ('btnSetRegion', 'btnSetRegionManual', 'btnClearRegion',
             'btnInfo', 'btnActivate', 'btnRefresh'):
    assert child(group_titles, name).get('bottom') == '-52', name
assert child(group_titles, 'use_default_title').get('bottom') == '-20'
assert child(group_titles, 'btnSetDefault').get('bottom') == '-18'
contact_add = resource('floater_fs_contact_add.xml')
assert int(contact_add.get('width')) == 380 and int(contact_add.get('height')) == 190
assert child(contact_add, 'textfield').get('right') == '-20'
assert int(child(contact_add, 'contact_sets').get('width')) == 300
display_name = resource('floater_display_name.xml')
assert int(display_name.get('width')) == 500
assert int(child(display_name, 'display_name_editor').get('width')) == 460
assert int(child(display_name, 'display_name_confirm').get('width')) == 460
assert all(int(child(display_name, name).get('height')) == 28
           for name in ('reset_btn', 'save_btn', 'cancel_btn'))
resident_picker = resource('floater_avatar_picker.xml')
assert int(resident_picker.get('width')) == 560
assert int(resident_picker.get('height')) == 460
resident_tabs = child(resident_picker, 'ResidentChooserTabs')
assert resident_tabs.get('left') == '16' and resident_tabs.get('right') == '-16'
for name in ('SearchPanel', 'FriendsPanel', 'NearMePanel', 'SearchPanelUUID', 'ContactSetsPanel'):
    panel = child(resident_tabs, name)
    assert panel.get('follows') == 'all' and panel.get('right') == '-4', name
for name in ('SearchResults', 'Friends', 'NearMe', 'SearchResultsUUID'):
    listing = child(resident_tabs, name)
    assert listing.get('right') == '-4' and int(listing.get('height')) >= 250, name
assert child(resident_tabs, 'Find').get('width') == '64'
assert child(resident_tabs, 'FindUUID').get('width') == '64'
for name in ('ok_btn', 'cancel_btn'):
    assert child(resident_picker, name).get('bottom') == '-42'
delete_preset = resource('floater_delete_pref_preset.xml')
assert int(delete_preset.get('width')) == 360 and int(delete_preset.get('height')) == 190
for name in ('delete', 'cancel'):
    assert child(delete_preset, name).get('follows') == 'right|bottom'
    assert child(delete_preset, name).get('bottom') == '-18'
search_replace = resource('floater_search_replace.xml')
assert int(search_replace.get('width')) == 500 and int(search_replace.get('height')) == 190
for name in ('search_btn', 'replace_btn', 'replace_all_btn'):
    assert child(search_replace, name).get('bottom') == '-18'
assert child(search_replace, 'search_text').get('follows') == 'left|top|right'
assert child(search_replace, 'replace_text').get('follows') == 'left|top|right'
teleport_history = resource('floater_fs_teleporthistory.xml')
assert int(teleport_history.get('width')) == 480
assert int(teleport_history.get('min_width')) == 460
history_placeholder = child(teleport_history, 'history_placeholder')
assert history_placeholder.get('left') == '16'
assert history_placeholder.get('right') == '-16'
assert history_placeholder.get('bottom') == '-48'
history_actions = child(teleport_history, 'button_panel')
assert history_actions.get('bottom') == '-12'
history_stack = child(history_actions, 'bottom_bar_ls0')
assert int(history_stack.get('width')) == 448
inner_history_stack = child(history_stack, 'show_lm_panel')
assert int(child(inner_history_stack, 'bottom_bar_ls1').get('width')) == 448
history_button_stack = child(inner_history_stack, 'bottom_bar_ls1')
for name, width in (('teleport_btn_lp', 132), ('chat_btn_lp', 112), ('profile_btn_lp', 172)):
    assert int(child(history_button_stack, name).get('width')) == width, name
profile_texture = resource('floater_profile_texture.xml')
assert profile_texture.get('bg_opaque_image') == 'Forme_Round_Window'

item = resource('sidepanel_item_info.xml')
assert child(item, 'CheckOwnerModify').get('left') != child(item, 'CheckNextOwnerModify').get('left')
assert child(item, 'item_scroll_container').find("./layout_stack[@name='main_stack']") is not None
move = resource('floater_moveview.xml')
for name in ['forward btn', 'backward btn', 'move left btn', 'move right btn']:
    assert child(move, name).tag in ['joystick_turn', 'joystick_slide'], name
# All eight directions and all three modes must fit the compact movement window,
# including for users whose previous version saved a much larger rectangle.
def fits(widget, parent):
    left, top, width, height = (int(widget.get(k)) for k in ('left', 'top', 'width', 'height'))
    assert 0 <= left < left + width <= int(parent.get('width')), widget.attrib
    assert 0 <= top < top + height <= int(parent.get('height')), widget.attrib

assert move.get('can_resize') == 'false' and int(move.get('width')) <= 280
assert int(move.get('min_height')) == int(move.get('height')) + int(move.get('header_height'))
directions = child(move, 'forme_layout_move_directions')
mode_stack = child(move, 'forme_layout_move_modes')
assert len(directions) == 8 and len(child(move, 'panel_modes')) == 3
for block in (directions, mode_stack):
    fits(block, move)
assert int(directions.get('left')) + int(directions.get('width')) < int(mode_stack.get('left'))
for button in directions:
    fits(button, directions)
for button in child(move, 'panel_modes'):
    fits(button, child(move, 'panel_modes'))
# Leave enough room for Stop Flying and Flycam to appear together.
assert int(child(move, 'modes_container').get('width')) >= 160
for skin in ('default', 'starlightcui'):
    themed = ET.parse(ROOT / f'indra/newview/skins/{skin}/xui/en/floater_moveview.xml').getroot()
    assert themed.attrib == move.attrib
movement_ru = ET.parse(BASE / 'xui/ru/floater_moveview.xml').getroot()
assert len(child(movement_ru, 'panel_modes').findall('button')) == 3
# The stock inventory starts in a narrow right-side window. Its four built-in tabs
# fit at minimum width, and custom tabs can still use native tab scrolling.
inv_floater = resource('floater_my_inventory.xml')
inv_tabs = child(inventory, 'inventory filter tabs')
inv_tree = child(inventory, 'default_inventory_panel')
assert int(inv_floater.get('width')) <= 400 and inv_floater.get('right') == '-16'
assert inv_floater.get('can_resize') == 'true' and inv_floater.get('save_rect') == 'true'
tab_room = (int(inv_floater.get('min_width')) - int(inv_tree.get('left')) + int(inv_tree.get('right'))
            - int(inv_tabs.get('left')) + int(inv_tabs.get('right')))
assert len(inv_tabs) * int(inv_tabs.get('tab_max_width')) + 16 <= tab_room
assert inv_tabs.get('right') == inv_tabs.get('bottom') == '-4'
category_scroll = child(inventory, 'forme_inventory_categories')
assert category_scroll.get('visible') == 'false'
assert int(category_scroll.get('width')) - int(child(inventory, 'forme_inventory_categories_body').get('width')) >= 20
assert child(inventory, 'filter_combo_box').find("./combo_box.item[@value='filter_type_custom']") is not None
assert child(inventory, 'forme_inventory_item_actions').get('enabled') == 'false'
assert child(inventory, 'forme_inventory_item_actions').get('right') == '-33'
assert child(people, 'panel_radar').get('right') == child(people, 'panel_radar').get('bottom') == '-1'
camera = resource('floater_camera.xml')
assert int(camera.get('min_height')) == int(camera.get('height')) + int(camera.get('header_height'))
assert camera.get('can_resize') == 'false' and int(camera.get('width')) <= 400
for block in ('buttons_panel', 'zoom', 'preset_buttons_panel'):
    fits(child(camera, block), camera)
for item in child(camera, 'buttons_panel'):
    fits(item, child(camera, 'buttons_panel'))
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
