#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Regressions for the photographed layout defects and compact native windows."""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
XUI = ROOT / 'indra/newview/skins/default/xui'

def resource(name, lang='en'):
    return ET.parse(XUI / lang / name).getroot()

def child(root, name):
    found = root.find(".//*[@name='%s']" % name)
    if found is None:
        raise AssertionError(name)
    return found

class CompactLayouts(unittest.TestCase):
    def test_settings_labels_stay_at_top_and_native_pages_can_scroll(self):
        window = resource('floater_preferences.xml')
        page = child(window, 'forme_interface')
        self.assertEqual('432', page.get('height'))
        for name in ('forme_interface_title', 'forme_scale_label', 'forme_scale', 'forme_font_label', 'forme_font'):
            self.assertIn('top', child(page, name).get('follows'))
        for page in child(window, 'pref core').findall('panel'):
            if not page.get('filename'):
                continue
            native = resource(page.get('filename'))
            viewport = child(native, 'compact_preferences_scroll')
            body = child(viewport, 'compact_preferences_body')
            self.assertGreater(int(body.get('height')), int(viewport.get('height')))
            self.assertNotIn('bottom', body.get('follows'))
            # Specialized preference classes remain the tab's root.
            self.assertTrue(page.get('class', '').startswith('panel_preference'))

    def test_profile_has_dark_editable_and_readonly_backgrounds(self):
        editor = child(resource('panel_profile_secondlife.xml'), 'sl_description_edit')
        for key in ('bg_readonly_color', 'bg_writeable_color', 'bg_focus_color'):
            color = [float(x) for x in editor.get(key).split()]
            self.assertLess(max(color[:3]), .3)
            self.assertEqual(1, color[3])
        translated = resource('panel_profile_secondlife.xml', 'ru')
        self.assertIsNotNone(translated.find("./panel[@name='forme_layout_profile_legacy']"))
        self.assertIsNone(translated.find("./layout_stack[@name='imagepositioner']"))

    def test_camera_controls_and_modes_do_not_overlap(self):
        camera = resource('floater_camera.xml')
        self.assertLessEqual(int(camera.get('width')), 340)
        sections = [child(camera, x) for x in ('buttons_panel', 'zoom', 'preset_buttons_panel', 'forme_camera_walk')]
        for previous, following in zip(sections, sections[1:]):
            self.assertLessEqual(int(previous.get('top')) + int(previous.get('height')), int(following.get('top')))
        for name in ('cam_rotate_stick', 'cam_track_stick'):
            control = child(camera, name)
            self.assertEqual('true', control.get('forme_ring'))
            self.assertEqual(control.get('width'), control.get('height'))
        for name in ('forme_view_front', 'forme_view_side', 'forme_view_back'):
            self.assertTrue(resource('floater_camera.xml', 'ru').find("floater.string[@name='%s']" % name).text.strip())

    def test_notifications_filter_is_outside_tab_buttons(self):
        window = resource('floater_notifications_tabbed.xml')
        filter_box = child(window, 'forme_unread_only')
        tabs = child(window, 'notifications_tab_container')
        self.assertLessEqual(int(filter_box.get('top')) + int(filter_box.get('height')), int(tabs.get('top')))
        title = child(resource('panel_notification_list_item.xml'), 'notification_title')
        self.assertEqual('true', title.get('word_wrap'))
        self.assertGreaterEqual(int(title.get('height')), 32)

    def test_new_sizes_ignore_previous_oversized_saved_rectangles(self):
        for name in ('floater_people.xml', 'floater_preferences.xml', 'floater_tools.xml', 'floater_profile.xml', 'floater_camera.xml'):
            self.assertTrue(resource(name).get('rect_control_suffix', '').endswith('v4'), name)
        minimap = resource('floater_map.xml')
        self.assertIsNone(minimap.get('bg_alpha_image_overlay'))
        self.assertEqual(1, float(child(minimap, 'Net Map').get('bg_color').split()[-1]))

    def test_chat_has_native_conversations_and_return_controls(self):
        child(resource('floater_fs_nearby_chat.xml'), 'forme_chat_dialogs')
        dialogs = resource('floater_fs_im_container.xml')
        for name in ('forme_nearby_chat', 'forme_new_dialog', 'im_box_tab_container'):
            child(dialogs, name)
        status = child(resource('panel_status_bar.xml'), 'forme_hud_balance')
        self.assertIn('refresh', status.get('tool_tip'))

if __name__ == '__main__':
    unittest.main()
