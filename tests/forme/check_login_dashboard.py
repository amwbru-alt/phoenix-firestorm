#!/usr/bin/env python3
"""Validate the ForMe login welcome hub geometry and locale overlay."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
XUI = ROOT / "indra/newview/skins/default/xui"

en = ET.parse(XUI / "en/panel_forme_login.xml").getroot()
ru = ET.parse(XUI / "ru/panel_forme_login.xml").getroot()

def child(root, name):
    node = root.find(".//*[@name='%s']" % name)
    assert node is not None, name
    return node

hub = child(en, "forme_welcome_hub")
assert hub.get("follows") == "right|top"
width, height = int(hub.get("width")), int(hub.get("height"))
cards = [child(hub, name) for name in (
    "forme_hub_explore_card", "forme_hub_connect_card", "forme_hub_create_card")]
rects = []
for card in cards:
    x, y = int(card.get("left")), int(card.get("top"))
    w, h = int(card.get("width")), int(card.get("height"))
    assert x >= 0 and y >= 0 and x + w <= width and y + h <= height
    rects.append((x, y, x + w, y + h))
for i, a in enumerate(rects):
    for b in rects[i + 1:]:
        assert a[2] <= b[0] or b[2] <= a[0] or a[3] <= b[1] or b[3] <= a[1]

for name in ("forme_hub_eyebrow", "forme_hub_title", "forme_hub_intro",
             "forme_hub_explore_title", "forme_hub_explore_copy",
             "forme_hub_connect_title", "forme_hub_connect_copy",
             "forme_hub_create_title", "forme_hub_create_copy", "forme_hub_footer"):
    child(hub, name)
    child(ru, name)

# The welcome area is informational; native credential and login controls remain intact.
for name in ("username_combo", "password_edit", "start_location_combo", "connect_btn"):
    child(en, name)
print("PASS ForMe login welcome hub: card bounds, spacing, locale overlay and native login controls")
