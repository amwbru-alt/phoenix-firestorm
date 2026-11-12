#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Protect existing forms and validate packaged rounded theme resources."""
from pathlib import Path
import hashlib
import json
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'indra/newview/skins/default'
contract = json.loads((ROOT / 'tests/forme/ui_functions_contract.json').read_text())
for name, expected in contract['resources'].items():
    root = ET.parse(BASE / 'xui/en' / name).getroot()
    def semantic_nodes(node):
        # Camera style parameter blocks configure existing constructor-owned
        # children; they do not add controls or change native command bindings.
        if name == 'floater_camera.xml' and node.tag in ('panel_camera_item.text', 'panel_camera_item.icon_over', 'panel_camera_item.icon_selected'):
            assert not any(key in node.attrib for key in contract['attributes'] if key != 'value')
            return []
        if node.get('name') == 'item_scroll_container' or node.get('name', '').startswith('forme_layout_'):
            return [row for child in node for row in semantic_nodes(child)]
        if node.get('name', '').startswith('forme_'):
            return []
        own = [(node.tag, {key: value for key, value in node.attrib.items()
                          if key in contract['attributes']})]
        return own + [row for child in node for row in semantic_nodes(child)]
    rows = semantic_nodes(root)
    rows.sort(key=lambda row: json.dumps(row, sort_keys=True, ensure_ascii=False))
    actual = hashlib.sha256(json.dumps(rows, sort_keys=True, ensure_ascii=False).encode()).hexdigest()
    assert actual == expected, f'{name}: native controls, bindings or commands changed'
textures = ET.parse(BASE / 'textures/textures.xml').getroot()
names = [node.get('name') for node in textures]
assert len(names) == len(set(names)), 'Duplicate texture alias'
rounded = [node for node in textures if node.get('file_name', '').startswith('forme_round/')]
assert len(rounded) >= 90
for node in rounded:
    path = BASE / 'textures' / node.get('file_name')
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', path
    width, height = struct.unpack('>II', data[16:24])
    if node.get('scale.left'):
        assert (width, height) == (64, 64), path
        assert 0 <= int(node.get('scale.left')) < int(node.get('scale.right')) <= width
        assert 0 <= int(node.get('scale.bottom')) < int(node.get('scale.top')) <= height
# The compact header toggles the original live menu, retaining runtime entries.
status = (ROOT / 'indra/newview/llstatusbar.cpp').read_text()
assert 'gMenuBarView->setVisible(full_menu)' in status
profile = ET.parse(BASE / 'xui/en/menu_forme_profile.xml').getroot()
assert ET.parse(BASE / "xui/en/floater_preferences.xml").find(".//*[@control_name='ForMeShowFullMenu']") is not None
native = ET.parse(BASE / 'xui/en/menu_fs_chat_options.xml').getroot()
compact = ET.parse(BASE / 'xui/en/menu_forme_chat_actions.xml').getroot()
for item in native:
    original = ET.tostring(item, encoding='unicode').strip()
    candidates = [ET.tostring(node, encoding='unicode').strip() for node in compact if node.tag == item.tag and node.get('name') == item.get('name')]
    # Whitespace and indentation are not functional.
    def semantic(xml):
        return [(node.tag, dict(node.attrib), (node.text or '').strip()) for node in ET.fromstring(xml).iter()]
    assert any(semantic(original) == semantic(candidate) for candidate in candidates), item.get('name')
print(f"PASS rounded theme: {len(rounded)} image aliases, {len(contract['resources'])} native forms preserve controls and bindings")
