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
             "category_objects", "category_animations", "category_favorites", "category_recent", "items_grid", "items_scroll",
             "details_surface", "detail_thumbnail", "detail_name", "detail_favorite", "apply", "open_folder",
             "previous", "next", "page", "folder_back", "status"):
    child(en, name)
    if name not in ("detail_icon", "items_scroll"):
        child(ru, name)

for index in range(20):
    child(en, "item_card_%d" % index)
    child(en, "item_thumbnail_%d" % index)
    child(en, "item_favorite_%d" % index)
    child(en, "item_name_%d" % index)
    child(en, "item_select_%d" % index)
    child(ru, "item_card_%d" % index)
    child(ru, "item_thumbnail_%d" % index)
    child(ru, "item_favorite_%d" % index)

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
assert "mFolder" in cpp and 'showInstance("inventory"' not in cpp
assert "category_clothing" in cpp and "mQuery" in cpp and "mPage" in cpp
assert "getThumbnailUUID()" in cpp and "toggle_favorite" in cpp and "mRecentItems" in cpp

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

# Independent Fitting Room: live Current Outfit Folder, take-off, inventory handoff and save.
fit_en = ET.parse(XUI / "en/floater_forme_fitting_room.xml").getroot()
fit_ru = ET.parse(XUI / "ru/floater_forme_fitting_room.xml").getroot()
assert fit_en.tag == "floater" and fit_en.get("name") == "forme_fitting_room"
assert fit_ru.tag == "floater" and fit_ru.get("name") == "forme_fitting_room"
for name in ("avatar_stage", "fitting_item_count", "fitting_open_inventory",
             "wardrobe_surface", "outfit_name", "fitting_save_outfit", "fitting_status"):
    child(fit_en, name)
for index in range(16):
    for suffix in ("card", "name", "type", "remove"):
        child(fit_en, "worn_" + suffix + "_%d" % index)
fit_cpp = (ROOT / "indra/newview/fsforme_fittingroom.cpp").read_text()
fit_header = (ROOT / "indra/newview/fsforme_fittingroom.h").read_text()
assert "LLInventoryObserver" in fit_header
assert "getDirectDescendentsOf" in fit_cpp and "getCOF()" in fit_cpp
assert "removeItemFromAvatar" in fit_cpp and "makeNewOutfitLinks" in fit_cpp
assert 'showInstance("forme_inventory")' in fit_cpp
assert 'LLFloaterReg::add("forme_fitting_room", "floater_forme_fitting_room.xml"' in registry
assert "fsforme_fittingroom.cpp" in cmake and "fsforme_fittingroom.h" in cmake
for locale in ("en", "ru"):
    sidebar = ET.parse(XUI / locale / "panel_forme_sidebar.xml").getroot()
    home = ET.parse(XUI / locale / "panel_sidetray_home_tab.xml").getroot()
    assert child(sidebar, "forme_sidebar_fitting_room").find("button.commit_callback").get("parameter") == "forme_fitting_room"
    assert child(home, "forme_home_open_fitting_room").find("button.commit_callback").get("parameter") == "forme_fitting_room"
print("PASS independent ForMe fitting room: live COF observer, take-off, inventory handoff, outfit save, locales and navigation")
