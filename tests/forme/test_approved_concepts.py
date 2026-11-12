#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Check the approved compact surfaces and their native fallback paths."""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
XUI = ROOT / 'indra/newview/skins/default/xui'


def resource(name):
    return ET.parse(XUI / 'en' / name).getroot()


def child(root, name):
    result = root.find(".//*[@name='%s']" % name)
    if result is None:
        raise AssertionError(name)
    return result


class ApprovedConcepts(unittest.TestCase):
    def test_profile_keeps_native_editor_separate(self):
        compact = resource('floater_profile.xml')
        self.assertEqual(420, int(compact.get('width')))
        for name in ('forme_profile_avatar', 'forme_profile_name', 'forme_profile_badge', 'forme_profile_message', 'forme_profile_details'):
            child(compact, name)
        main = resource('panel_profile_secondlife.xml')
        legacy = child(main, 'forme_layout_profile_legacy')
        self.assertEqual('false', legacy.get('visible'))
        self.assertIsNotNone(main.find("./text_editor[@name='sl_description_edit']"))
        self.assertIsNotNone(main.find("./group_list[@name='group_list']"))
        full = resource('floater_profile_details.xml')
        self.assertEqual('panel_profile_secondlife_details.xml', child(full, 'panel_profile_secondlife').get('filename'))
        detail = resource('panel_profile_secondlife_details.xml')
        for name in ('show_on_map_btn', 'pay', 'teleport', 'im', 'group_invite', 'copy_btn', 'set_name'):
            child(detail, name)

    def test_locale_overlays_follow_moved_profile_controls(self):
        for path in XUI.glob('*/panel_profile_secondlife.xml'):
            if path.parent.name == 'en':
                continue
            root = ET.parse(path).getroot()
            self.assertIsNotNone(root.find("./panel[@name='forme_layout_profile_legacy']"), str(path))
            self.assertIsNone(root.find("./layout_stack[@name='imagepositioner']"), str(path))
            self.assertTrue((path.parent / 'panel_profile_secondlife_details.xml').exists())

    def test_settings_have_real_bindings_and_cancel_snapshot(self):
        root = resource('floater_preferences.xml')
        interface = child(root, 'forme_interface')
        for name, setting in [('forme_scale', 'UIScaleFactor'), ('forme_font', 'FSFontSizeAdjustment'), ('forme_tooltips', 'ShowHoverTips'), ('forme_full_menu_binding', 'ForMeShowFullMenu')]:
            self.assertEqual(setting, child(interface, name).get('control_name'))
        panels = {p.get('name') for p in child(root, 'pref core').findall('panel')}
        more = child(root, 'forme_preferences_more')
        self.assertTrue({item.get('value') for item in more.findall('combo_box.item')}.issubset(panels))
        self.assertEqual('Pref.Apply', child(root, 'forme_apply').find('button.commit_callback').get('function'))

    def test_camera_preserves_native_joysticks_and_has_three_real_modes(self):
        root = resource('floater_camera.xml')
        for name, tag in [('cam_rotate_stick', 'joystick_rotate'), ('cam_track_stick', 'joystick_track')]:
            control = child(root, name)
            self.assertEqual(tag, control.tag)
            self.assertEqual('true', control.get('forme_ring'))
        for name in ('walk', 'run', 'fly', 'reset'):
            child(root, 'forme_camera_' + name)
        zoom, presets = child(root, 'zoom'), child(root, 'preset_buttons_panel')
        self.assertLessEqual(int(zoom.get('top')) + int(zoom.get('height')), int(presets.get('top')))

    def test_notification_viewports_and_read_controls(self):
        root = resource('floater_notifications_tabbed.xml')
        for name in ('system_notification_list', 'group_invite_notification_list'):
            view = child(root, name)
            self.assertNotEqual('0', view.get('height'))
            self.assertEqual('-1', view.get('bottom'))
        child(root, 'forme_unread_only')
        child(root, 'forme_mark_read')
        self.assertEqual('false', child(root, 'DeleteAllButtonPanel').get('visible'))


if __name__ == '__main__':
    unittest.main()
