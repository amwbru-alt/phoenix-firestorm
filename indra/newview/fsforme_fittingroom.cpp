/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "fsforme_fittingroom.h"
#include "llappearancemgr.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "llthumbnailctrl.h"
#include "llinventoryicon.h"
#include "llviewerinventory.h"
namespace
{
constexpr S32 WORN_ITEM_SLOTS = 16;
std::string fittingItemType(const LLViewerInventoryItem* item)
{
    if (!item) return "Предмет";
    if (item->getType() == LLAssetType::AT_CLOTHING) return "Одежда";
    if (item->getType() == LLAssetType::AT_BODYPART) return "Часть тела";
    if (item->getType() == LLAssetType::AT_OBJECT) return "Аксессуар / объект";
    return "Другой предмет";
}
}
FSForMeFittingRoom::FSForMeFittingRoom(const LLSD& key) : LLFloater(key) {}
FSForMeFittingRoom::~FSForMeFittingRoom() { gInventory.removeObserver(this); }
bool FSForMeFittingRoom::postBuild()
{
    gInventory.addObserver(this);
    getChild<LLButton>("fitting_refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { mDirty = true; });
    getChild<LLButton>("fitting_camera_front")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(CAMERA_PRESET_FRONT_VIEW);
    });
    getChild<LLButton>("fitting_camera_rear")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(CAMERA_PRESET_REAR_VIEW);
    });
    getChild<LLButton>("fitting_camera_three_quarter")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(CAMERA_PRESET_GROUP_VIEW);
    });
    getChild<LLButton>("fitting_camera_reset")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(CAMERA_PRESET_REAR_VIEW);
        gAgentCamera.setCameraZoomFraction(0.5f);
    });
    getChild<LLButton>("fitting_camera_zoom_out")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        adjustCameraZoom(-0.12f);
    });
    getChild<LLButton>("fitting_camera_zoom_in")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        adjustCameraZoom(0.12f);
    });
    getChild<LLButton>("fitting_open_inventory")->setClickedCallback([](LLUICtrl*, const LLSD&) {
        LLFloaterReg::showInstance("forme_inventory");
    });
    getChild<LLButton>("fitting_save_outfit")->setClickedCallback([this](LLUICtrl*, const LLSD&) { saveOutfit(); });
    getChild<LLButton>("fitting_wear_saved")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        const LLUUID outfit_id = getChild<LLComboBox>("saved_outfits")->getSelectedValue().asUUID();
        if (auto* outfit = gInventory.getCategory(outfit_id))
            LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(outfit, false);
    });
    for (S32 i = 0; i < WORN_ITEM_SLOTS; ++i)
        getChild<LLButton>("worn_remove_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            removeWornItem(i);
        });
    return true;
}
void FSForMeFittingRoom::onOpen(const LLSD&)
{
    mPreviousCameraMode = static_cast<S32>(gAgentCamera.getCameraMode());
    mPreviousCameraPreset = static_cast<S32>(gAgentCamera.getCameraPreset());
    mPreviousCameraZoom = gAgentCamera.getCameraZoomFraction();
    mCameraStateSaved = true;

    // This is a camera-control bridge to the live world avatar, not an
    // embedded/off-screen viewport. The independent preview renderer remains
    // a separate implementation task.
    gAgentCamera.changeCameraToThirdPerson(true);
    gAgentCamera.setFocusOnAvatar(true, true);
    gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
    gAgentCamera.setCameraZoomFraction(0.5f);
    mDirty = true;
}
void FSForMeFittingRoom::onClose(bool app_quitting)
{
    restoreCameraState();
}
void FSForMeFittingRoom::setCameraPreset(S32 preset)
{
    if (!gAgentAvatarp) return;
    gAgentCamera.changeCameraToThirdPerson(true);
    gAgentCamera.setFocusOnAvatar(true, true);
    gAgentCamera.switchCameraPreset(static_cast<ECameraPreset>(preset));
}
void FSForMeFittingRoom::adjustCameraZoom(F32 delta)
{
    const F32 zoom = gAgentCamera.getCameraZoomFraction();
    gAgentCamera.setCameraZoomFraction(llclamp(zoom + delta, 0.f, 1.f));
}
void FSForMeFittingRoom::restoreCameraState()
{
    if (!mCameraStateSaved) return;
    gAgentCamera.switchCameraPreset(static_cast<ECameraPreset>(mPreviousCameraPreset));
    switch (static_cast<ECameraMode>(mPreviousCameraMode))
    {
        case CAMERA_MODE_MOUSELOOK: gAgentCamera.changeCameraToMouselook(true); break;
        case CAMERA_MODE_CUSTOMIZE_AVATAR: gAgentCamera.changeCameraToCustomizeAvatar(); break;
        case CAMERA_MODE_FOLLOW: gAgentCamera.changeCameraToFollow(true); break;
        case CAMERA_MODE_THIRD_PERSON:
        default: gAgentCamera.changeCameraToThirdPerson(true); break;
    }
    gAgentCamera.setCameraZoomFraction(mPreviousCameraZoom);
    mCameraStateSaved = false;
}
void FSForMeFittingRoom::changed(U32) { mDirty = true; }
void FSForMeFittingRoom::draw()
{
    if (mDirty) rebuild();
    LLFloater::draw();
}
void FSForMeFittingRoom::rebuild()
{
    mDirty = false;
    mWornItems.clear();
    auto* outfits_combo = getChild<LLComboBox>("saved_outfits");
    outfits_combo->removeall();
    mSavedOutfitCount = 0;
    const LLUUID outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
    LLInventoryModel::cat_array_t* outfit_categories = nullptr;
    LLInventoryModel::item_array_t* outfit_items = nullptr;
    gInventory.getDirectDescendentsOf(outfits_id, outfit_categories, outfit_items);
    if (outfit_categories)
        for (const auto& outfit : *outfit_categories)
            if (outfit.notNull()) { outfits_combo->add(outfit->getName(), outfit->getUUID()); ++mSavedOutfitCount; }
    getChild<LLButton>("fitting_wear_saved")->setEnabled(mSavedOutfitCount > 0);
    LLInventoryModel::cat_array_t* categories = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(LLAppearanceMgr::instance().getCOF(), categories, items);
    if (items)
        for (const auto& item : *items)
            if (item.notNull() && item->getIsLinkType()) mWornItems.push_back(item->getUUID());
    for (S32 i = 0; i < WORN_ITEM_SLOTS; ++i)
    {
        const bool present = i < static_cast<S32>(mWornItems.size());
        getChild<LLPanel>("worn_card_" + std::to_string(i))->setVisible(present);
        if (!present) continue;
        const auto* item = gInventory.getItem(mWornItems[i]);
        if (!item) continue;
        getChild<LLTextBox>("worn_name_" + std::to_string(i))->setText(item->getName());
        getChild<LLTextBox>("worn_type_" + std::to_string(i))->setText(fittingItemType(item));
    }
    getChild<LLTextBox>("fitting_item_count")->setText(std::to_string(mWornItems.size()) + " предметов надето");
    getChild<LLTextBox>("fitting_status")->setText(mWornItems.empty()
        ? "Ожидаем данные текущего образа. Если аватар ещё загружается, обновите список позже."
        : "Список синхронизирован с текущим образом аватара.");
}
void FSForMeFittingRoom::removeWornItem(S32 index)
{
    if (index < 0 || index >= static_cast<S32>(mWornItems.size())) return;
    LLAppearanceMgr::instance().removeItemFromAvatar(mWornItems[index]);
    mDirty = true;
}
void FSForMeFittingRoom::saveOutfit()
{
    std::string name = getChild<LLLineEditor>("outfit_name")->getValue().asString();
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        getChild<LLTextBox>("fitting_status")->setText("Введите название комплекта перед сохранением.");
        return;
    }
    const auto last = name.find_last_not_of(" \t\r\n");
    name = name.substr(first, last - first + 1);
    LLAppearanceMgr::instance().makeNewOutfitLinks(name, false);
    getChild<LLTextBox>("fitting_status")->setText("Создаём комплект «" + name + "» в папке «Мои образы»…");
}
