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
    assert child(sidebar, "forme_sidebar_inventory") is not None
    assert child(home, "forme_home_open_inventory") is not None
assert child(ET.parse(XUI / "en/panel_forme_sidebar.xml").getroot(), "forme_sidebar_inventory").find("button.commit_callback").get("parameter") == "forme_inventory"
assert child(ET.parse(XUI / "en/panel_sidetray_home_tab.xml").getroot(), "forme_home_open_inventory").find("button.commit_callback").get("parameter") == "forme_inventory"

print("PASS independent ForMe inventory workspace: native model observer, search, categories, paging, apply, locales and navigation")

# Independent Fitting Room: live Current Outfit Folder, take-off, inventory handoff and save.
fit_en = ET.parse(XUI / "en/floater_forme_fitting_room.xml").getroot()
fit_ru = ET.parse(XUI / "ru/floater_forme_fitting_room.xml").getroot()
assert fit_en.tag == "floater" and fit_en.get("name") == "forme_fitting_room"
assert fit_ru.tag == "floater" and fit_ru.get("name") == "forme_fitting_room"
for name in ("avatar_stage", "fitting_item_count", "fitting_open_inventory", "worn_filter_all", "worn_filter_clothing", "worn_filter_shape", "worn_filter_skin", "worn_filter_hair", "worn_filter_eyes", "worn_filter_accessories",
             "wardrobe_surface", "saved_outfits", "fitting_wear_saved", "fitting_move_saved_to_trash", "compare_outfit_a", "compare_outfit_b", "fitting_compare_outfits", "outfit_name", "fitting_save_outfit", "fitting_status"):
    child(fit_en, name)
for index in range(16):
    for suffix in ("card", "name", "type", "remove", "thumbnail"):
        child(fit_en, "worn_" + suffix + "_%d" % index)
fit_cpp = (ROOT / "indra/newview/fsforme_fittingroom.cpp").read_text()
fit_header = (ROOT / "indra/newview/fsforme_fittingroom.h").read_text()
assert "LLInventoryObserver" in fit_header
assert "getDirectDescendentsOf" in fit_cpp and "getCOF()" in fit_cpp
assert "removeItemFromAvatar" in fit_cpp and "makeNewOutfitLinks" in fit_cpp
assert "wearInventoryCategoryOnAvatar" in fit_cpp and "FT_MY_OUTFITS" in fit_cpp
assert "fitting_move_saved_to_trash" in fit_cpp and "gInventory.removeCategory(outfit_id)" in fit_cpp
assert "compareInsensitive" in fit_cpp and "sorted_outfits" in fit_cpp
assert "saved_outfit_filter" in fit_cpp and "setKeystrokeCallback" in fit_cpp
assert "updateSelectedOutfitDetails" in fit_cpp
assert "saved_outfit_details" in fit_cpp
assert "mDetailsOutfit" in fit_cpp
assert "saved_outfit_filter_clear" in fit_cpp and 'setText("", true)' in fit_cpp
assert "utf8str_tolower(outfit->getName()).find(outfit_filter)" in fit_cpp
assert "utf8str_tolower(item->getName()).find(outfit_filter)" in fit_cpp
assert "matches_filter" in fit_cpp
assert "По запросу не найдено сохранённых образов" in fit_cpp
assert "const S32 item_count" in fit_cpp and "std::to_string(item_count)" in fit_cpp
assert "Образ с таким названием уже существует" in fit_cpp
assert "Выбранный образ недоступен" in fit_cpp
assert "outfit->getParentUUID() != outfits_root" in fit_cpp
assert "compareOutfits()" in fit_cpp and "getLinkedUUID()" in fit_cpp
assert 'getChild<LLComboBox>("compare_outfit_a")->setCommitCallback' in fit_cpp
assert 'getChild<LLComboBox>("compare_outfit_b")->setCommitCallback' in fit_cpp
assert "setCurrentByID(previous_outfit)" in fit_cpp
assert "setCurrentByID(previous_compare_a)" in fit_cpp and "setCurrentByID(previous_compare_b)" in fit_cpp
assert "std::set<LLUUID>" in fit_cpp
assert "collect_names" in fit_cpp and "only_first_names" in fit_cpp and "only_second_names" in fit_cpp
assert 'getTypedInstance<LLFloaterSnapshot>("snapshot")' in fit_cpp
assert "llfloatersnapshot.h" in fit_cpp
assert "отдельный захват preview-рендера пока не подключён" not in fit_cpp
assert "requestCapture" in fit_cpp and "glReadPixels" in fit_cpp
assert "formeLookbookDirectory" in fit_cpp and "saveFormeLookbook" in fit_cpp
assert "LLSDSerialize::toPrettyXML" in fit_cpp and "LLSDSerialize::fromXML" in fit_cpp
assert "outfit_id" in fit_cpp and "camera_yaw" in fit_cpp and "captured_at" in fit_cpp
assert "lookbook_delete" in fit_cpp and "LLFile::remove" in fit_cpp
assert "getFetchedTextureFromFile" in fit_cpp and "drawLookbookThumbnails" in fit_cpp
assert "drawLookbookPhotoPreview" in fit_cpp
assert "drawLookbookComparison" in fit_cpp and "saveSelectedLookbookMetadata" in fit_cpp
assert "lookbook_filter_category" in fit_cpp and "mLookbookFilterCategory" in fit_cpp
assert "mFilteredLookbookEntries" in fit_cpp and "mLookbookFavoritesOnly" in fit_cpp
assert 'entry["favorite"] = false' in fit_cpp and '["favorite"].asBoolean()' in fit_cpp
assert "lookbook_toggle_favorite" in fit_cpp and "lookbook_favorites_only" in fit_cpp
assert "lookbook_wear_outfit" in fit_cpp and "wearInventoryCategoryOnAvatar(outfit, false)" in fit_cpp
assert "lookbook_fullscreen" in fit_cpp and "mLookbookFullscreenMode" in fit_cpp
assert "handleKeyHere(KEY key, MASK mask)" in fit_cpp and "KEY_LEFT" in fit_cpp and "KEY_RIGHT" in fit_cpp and "KEY_ESCAPE" in fit_cpp
assert "navigateLookbookPhoto" in fit_cpp and "setLookbookFullscreenMode" in fit_cpp
assert "mLookbookOldestFirst" in fit_cpp and "lookbook_sort_order" in fit_cpp
assert "lookbook_history_date" in fit_cpp and "captured_at" in fit_cpp
assert "assignLookbookCompareSlot" in fit_cpp and 'entry["category"] = "Everyday"' in fit_cpp
assert 'entry["notes"] = ""' in fit_cpp and 'entry["title"] = outfit_name' in fit_cpp
child(fit_en, "lookbook_thumb_3")
assert "lookbook_entries" not in (XUI / "en/floater_forme_fitting_room.xml").read_text()
assert "png->save(image_path)" in fit_cpp
assert "GL_PACK_ALIGNMENT" in fit_cpp and "verticalFlip" in fit_cpp
assert "LLImagePNG" in fit_cpp and "png->save(image_path)" in fit_cpp
assert "compare_summary" in fit_cpp and "compare_only_a" in fit_cpp and "compare_only_b" in fit_cpp
assert "const S32 omitted = total - static_cast<S32>(names.size())" in fit_cpp
assert "Сравнение готово" in fit_cpp
assert "names.size() < 4" in fit_cpp
assert "WORN_ITEM_SLOTS" in fit_cpp and "page_count" in fit_cpp
assert "page_start" in fit_cpp and "mWornPage" in fit_cpp
assert "mWornPage = 0" in fit_cpp and "mCategory == category" in fit_cpp
assert "worn_page_previous" in fit_cpp and "worn_page_next" in fit_cpp
assert "fitting_compare_outfits" in fit_cpp and "compare_outfit_a" in fit_cpp and "compare_outfit_b" in fit_cpp
assert "matchesCategory" in fit_cpp and "setCategory" in fit_cpp
assert "worn_filter_shape" in fit_cpp and "worn_filter_skin" in fit_cpp
assert "worn_filter_hair" in fit_cpp and "worn_filter_eyes" in fit_cpp
assert "getWearableType()" in fit_cpp and "LLWearableType::WT_SKIN" in fit_cpp
assert "LLWearableType::WT_SHIRT" in fit_cpp and "LLWearableType::WT_PANTS" in fit_cpp
assert "LLWearableType::WT_SHOES" in fit_cpp and "LLWearableType::WT_UNIVERSAL" in fit_cpp
assert 'showInstance("forme_inventory")' in fit_cpp
assert '#include "lldynamictexture.h"' in fit_cpp
assert "LLViewerDynamicTexture(512, 307" in fit_cpp
assert "setViewNoBroadcast(0.78539816f)" in fit_cpp
assert "generateImpostor(avatar, true, false)" in fit_cpp
assert "localRectToOtherView" in fit_cpp
assert "setPerspective(false, 0, 0, getWidth(), getHeight()" in fit_cpp
assert "setOriginAndLookAt" in fit_cpp and "needsRender() override" in fit_cpp
assert "avatar->mDrawable.isNull()" in fit_cpp
assert "setPreviewActive(!isMinimized())" in fit_cpp
assert fit_en.get("height") == "730" and fit_ru.get("height") == "730"
assert fit_en.get("min_width") == "980" and fit_ru.get("min_width") == "980"
assert fit_en.get("min_height") == "730" and fit_ru.get("min_height") == "730"
assert "gAgentCamera" not in fit_cpp
assert "handleMouseDown" in fit_cpp and "handleMouseUp" in fit_cpp
assert "const std::string outfit_filter =" in fit_cpp
assert "MASK_LEFT" not in fit_cpp
assert "handleHover" in fit_cpp and "handleScrollWheel" in fit_cpp
assert "mAvatarPreview->rotate" in fit_cpp and "mAvatarPreview->tilt" in fit_cpp
assert "adjustCameraZoom" in fit_cpp and "mLastPreviewMouseY" in fit_cpp
assert "FSForMeAvatarPreview* mAvatarPreview" in fit_header
assert "LLThumbnailCtrl" in fit_cpp and "getThumbnailUUID()" in fit_cpp
for locale in ("en", "ru"):
    fit_ui = ET.parse(XUI / locale / "floater_forme_fitting_room.xml").getroot()
    canvas = child(fit_ui, "avatar_preview_canvas")
    for name in ("worn_filter_all", "worn_filter_clothing", "worn_filter_shape", "worn_filter_skin", "worn_filter_hair", "worn_filter_eyes", "worn_filter_accessories"):
        child(fit_ui, name)
    child(fit_ui, "avatar_preview_loading")
    child(fit_ui, "compare_outfit_a")
    child(fit_ui, "compare_outfit_b")
    child(fit_ui, "fitting_compare_outfits")
    child(fit_ui, "fitting_open_photo_studio")
    child(fit_ui, "fitting_capture_preview")
    child(fit_ui, "lookbook_thumb_0")
    child(fit_ui, "lookbook_thumb_1")
    child(fit_ui, "lookbook_thumb_2")
    child(fit_ui, "lookbook_thumb_3")
    child(fit_ui, "lookbook_prev")
    child(fit_ui, "lookbook_next")
    child(fit_ui, "lookbook_view_avatar")
    child(fit_ui, "lookbook_page_label")
    child(fit_ui, "lookbook_delete")
    child(fit_ui, "lookbook_title")
    child(fit_ui, "lookbook_notes")
    child(fit_ui, "lookbook_category")
    child(fit_ui, "lookbook_filter_category")
    child(fit_ui, "lookbook_favorites_only")
    child(fit_ui, "lookbook_toggle_favorite")
    child(fit_ui, "lookbook_wear_outfit")
    child(fit_ui, "lookbook_fullscreen")
    child(fit_ui, "lookbook_sort_order")
    child(fit_ui, "lookbook_history_date")
    child(fit_ui, "lookbook_save_metadata")
    child(fit_ui, "lookbook_compare_a")
    child(fit_ui, "lookbook_compare_b")
    child(fit_ui, "lookbook_compare")
    assert child(fit_ui, "lookbook_title").get("width") == "130"
    assert child(fit_ui, "lookbook_filter_category").get("top") == "34"
    assert child(fit_ui, "lookbook_favorites_only").get("top") == "34"
    assert child(fit_ui, "lookbook_toggle_favorite").get("width") == "72"
    assert child(fit_ui, "fitting_capture_preview").get("top") == "466"
    assert child(fit_ui, "avatar_stage").get("height") == "630"
    child(fit_ui, "fitting_capture_preview")
    child(fit_ui, "compare_summary")
    child(fit_ui, "compare_only_a")
    child(fit_ui, "compare_only_b")
    child(fit_ui, "fitting_move_saved_to_trash")
    child(fit_ui, "saved_outfit_filter")
    child(fit_ui, "saved_outfit_filter_clear")
    child(fit_ui, "saved_outfit_details")
    child(fit_ui, "worn_page_previous")
    child(fit_ui, "worn_page_next")
    child(fit_ui, "worn_page_label")
    assert canvas.get("width") == "350" and canvas.get("height") == "210"
    for name in ("fitting_camera_front", "fitting_camera_rear", "fitting_camera_three_quarter",
                 "fitting_camera_reset", "fitting_camera_zoom_out", "fitting_camera_zoom_in",
                 "fitting_camera_rotate_left", "fitting_camera_rotate_right"):
        child(fit_ui, name)
    for index in range(16):
        child(fit_ui, "worn_thumbnail_%d" % index)
assert 'LLFloaterReg::add("forme_fitting_room", "floater_forme_fitting_room.xml"' in registry
assert "fsforme_fittingroom.cpp" in cmake and "fsforme_fittingroom.h" in cmake
for locale in ("en", "ru"):
    sidebar = ET.parse(XUI / locale / "panel_forme_sidebar.xml").getroot()
    home = ET.parse(XUI / locale / "panel_sidetray_home_tab.xml").getroot()
    assert child(sidebar, "forme_sidebar_fitting_room") is not None
    assert child(home, "forme_home_open_fitting_room") is not None
assert child(ET.parse(XUI / "en/panel_forme_sidebar.xml").getroot(), "forme_sidebar_fitting_room").find("button.commit_callback").get("parameter") == "forme_fitting_room"
assert child(ET.parse(XUI / "en/panel_sidetray_home_tab.xml").getroot(), "forme_home_open_fitting_room").find("button.commit_callback").get("parameter") == "forme_fitting_room"
print("PASS independent ForMe fitting room: live COF observer, take-off, inventory handoff, outfit save, locales and navigation")
