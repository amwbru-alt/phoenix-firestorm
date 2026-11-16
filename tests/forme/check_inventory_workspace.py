#!/usr/bin/env python3
"""Guard the independent ForMe inventory workspace and its live-model adapter."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
XUI = ROOT / "indra/newview/skins/default/xui"
en = ET.parse(XUI / "en/floater_forme_inventory.xml").getroot()
ru = ET.parse(XUI / "ru/floater_forme_inventory.xml").getroot()
assert en.tag == "floater" and en.get("name") == "forme_inventory"
assert ru.tag == "floater" and ru.get("name") == "forme_inventory"

def child(root, name):
    node = root.find(".//*[@name='%s']" % name)
    assert node is not None, name
    return node

for name in ("query", "category_all", "category_clothing", "category_accessories",
             "category_objects", "category_animations", "items_grid", "items_scroll",
             "details_surface", "detail_icon", "detail_name", "apply", "open_folder",
             "previous", "next", "page", "status"):
    child(en, name)
    if name not in ("detail_icon", "items_scroll"):
        child(ru, name)

for index in range(20):
    child(en, "item_card_%d" % index)
    child(en, "item_icon_%d" % index)
    child(en, "item_name_%d" % index)
    child(en, "item_select_%d" % index)
    child(ru, "item_card_%d" % index)

# This is a new ForMe surface, not an embedding or mutation of the legacy inventory UI.
for node in en.iter():
    assert node.tag not in ("inventory_panel", "folder_view", "web_browser")
assert not any("panel_main_inventory" in (node.get("filename") or "") for node in en.iter())

cpp = (ROOT / "indra/newview/fsformeinventory.cpp").read_text()
header = (ROOT / "indra/newview/fsformeinventory.h").read_text()
assert "LLInventoryObserver" in header
assert "gInventory.addObserver(this)" in cpp and "gInventory.removeObserver(this)" in cpp
assert "getDirectDescendentsOf" in cpp
assert "wearItemOnAvatar" in cpp
assert "category_clothing" in cpp and "mQuery" in cpp and "mPage" in cpp

registry = (ROOT / "indra/newview/llviewerfloaterreg.cpp").read_text()
cmake = (ROOT / "indra/newview/CMakeLists.txt").read_text()
assert 'LLFloaterReg::add("forme_inventory", "floater_forme_inventory.xml"' in registry
assert "fsformeinventory.cpp" in cmake and "fsformeinventory.h" in cmake

for locale in ("en", "ru"):
    sidebar = ET.parse(XUI / locale / "panel_forme_sidebar.xml").getroot()
    home = ET.parse(XUI / locale / "panel_sidetray_home_tab.xml").getroot()
    assert child(sidebar, "forme_sidebar_inventory").find("button.commit_callback").get("parameter") == "forme_inventory"
    assert child(home, "forme_home_open_inventory").find("button.commit_callback").get("parameter") == "forme_inventory"

print("PASS independent ForMe inventory workspace: native model observer, search, categories, paging, apply, locales and navigation")
