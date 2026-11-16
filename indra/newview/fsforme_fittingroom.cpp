/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "fsforme_fittingroom.h"
#include "llappearancemgr.h"
#include "lldynamictexture.h"
#include "llviewercamera.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llfocusmgr.h"
#include "llviewershadermgr.h"
#include "llrender.h"
#include "pipeline.h"
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
class FSForMeAvatarPreview final : public LLViewerDynamicTexture
{
public:
    FSForMeAvatarPreview()
        : LLViewerDynamicTexture(512, 512, 3, ORDER_MIDDLE, false)
    {
    }

    ~FSForMeAvatarPreview() override = default;

    bool needsRender() override { return mActive; }
    void setActive(bool active) { mActive = active; }
    void setYaw(F32 yaw) { mYaw = yaw; }
    void rotate(F32 delta) { mYaw += delta; }
    void tilt(F32 delta) { mElevation = llclamp(mElevation + delta, -0.35f, 0.65f); }
    void adjustDistance(F32 delta) { mDistance = llclamp(mDistance + delta, 1.8f, 7.0f); }
    void resetView() { mYaw = 0.f; mDistance = 3.4f; mElevation = 0.18f; }

    bool render() override
    {
        LLVOAvatar* avatar = gAgentAvatarp;
        if (!mActive || !avatar || avatar->isDead())
            return false;

        LLViewerCamera* camera = LLViewerCamera::getInstance();
        camera->setAspect(static_cast<F32>(getWidth()) / static_cast<F32>(getHeight()));
        camera->setViewNoBroadcast(camera->getDefaultFOV());

        const LLVector3 focus = avatar->getPositionAgent() + LLVector3(0.f, 0.f, 1.0f);
        const LLQuaternion orbit = avatar->getRotation() * LLQuaternion(mYaw, LLVector3::z_axis);
        const LLVector3 offset = LLVector3(-mDistance, 0.f, mDistance * mElevation) * orbit;
        camera->setOriginAndLookAt(focus + offset, LLVector3::z_axis, focus);
        camera->setPerspective(false, 0, 0, getWidth(), getHeight(), false, 0.1f, 64.f);

        glClearColor(0.035f, 0.025f, 0.05f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        LLGLSUIDefault state;
        LLGLDepthTest depth(GL_TRUE);
        LLGLEnable cull(GL_CULL_FACE);
        // Use the viewer's dedicated avatar-preview pipeline. It marks the
        // avatar and its attachment bridges visible, hides unrelated world
        // geometry, and renders rigged attachments into the currently bound
        // dynamic-texture target. The dynamic-texture lifecycle restores the
        // world camera and viewport after this call.
        gPipeline.generateImpostor(avatar, true, false);
        gGL.flush();
        return true;
    }

private:
    bool mActive = false;
    F32 mYaw = 0.f;
    F32 mDistance = 3.4f;
    F32 mElevation = 0.18f;
};

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
FSForMeFittingRoom::~FSForMeFittingRoom() { gInventory.removeObserver(this); delete mAvatarPreview; }
bool FSForMeFittingRoom::postBuild()
{
    gInventory.addObserver(this);
    mAvatarPreview = new FSForMeAvatarPreview();
    getChild<LLButton>("fitting_refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { mDirty = true; });
    getChild<LLButton>("fitting_camera_front")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(3.14159265f);
    });
    getChild<LLButton>("fitting_camera_rear")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(0.f);
    });
    getChild<LLButton>("fitting_camera_three_quarter")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setCameraPreset(0.78539816f);
    });
    getChild<LLButton>("fitting_camera_reset")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->resetView();
    });
    getChild<LLButton>("fitting_camera_zoom_out")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        adjustCameraZoom(0.35f);
    });
    getChild<LLButton>("fitting_camera_zoom_in")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        adjustCameraZoom(-0.35f);
    });
    getChild<LLButton>("fitting_camera_rotate_left")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->rotate(-0.26179939f);
    });
    getChild<LLButton>("fitting_camera_rotate_right")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->rotate(0.26179939f);
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
    if (mAvatarPreview) mAvatarPreview->setActive(true);
    mDirty = true;
}
void FSForMeFittingRoom::onClose(bool)
{
    mDraggingPreview = false;
    if (hasMouseCapture()) gFocusMgr.setMouseCapture(nullptr);
    if (mAvatarPreview) mAvatarPreview->setActive(false);
}
void FSForMeFittingRoom::setCameraPreset(F32 preset)
{
    if (mAvatarPreview) mAvatarPreview->setYaw(preset);
}
void FSForMeFittingRoom::adjustCameraZoom(F32 delta)
{
    if (mAvatarPreview) mAvatarPreview->adjustDistance(delta);
}
bool FSForMeFittingRoom::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect canvas_rect;
    if ((mask & MASK_LEFT) && canvas && canvas->localRectToOtherView(canvas->getLocalRect(), &canvas_rect, this)
        && canvas_rect.pointInRect(x, y) && mAvatarPreview)
    {
        mDraggingPreview = true;
        mLastPreviewMouseX = x;
        mLastPreviewMouseY = y;
        gFocusMgr.setMouseCapture(this);
        bringToFront(x, y);
        return true;
    }
    return LLFloater::handleMouseDown(x, y, mask);
}

bool FSForMeFittingRoom::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (mDraggingPreview)
    {
        mDraggingPreview = false;
        if (hasMouseCapture())
        {
            gFocusMgr.setMouseCapture(nullptr);
        }
        return true;
    }
    return LLFloater::handleMouseUp(x, y, mask);
}

bool FSForMeFittingRoom::handleHover(S32 x, S32 y, MASK mask)
{
    if (mDraggingPreview && hasMouseCapture() && mAvatarPreview)
    {
        const S32 delta_x = x - mLastPreviewMouseX;
        const S32 delta_y = y - mLastPreviewMouseY;
        mLastPreviewMouseX = x;
        mLastPreviewMouseY = y;
        mAvatarPreview->rotate(-static_cast<F32>(delta_x) * 0.012f);
        mAvatarPreview->tilt(-static_cast<F32>(delta_y) * 0.008f);
        return true;
    }
    return LLFloater::handleHover(x, y, mask);
}

bool FSForMeFittingRoom::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect canvas_rect;
    if (canvas && canvas->localRectToOtherView(canvas->getLocalRect(), &canvas_rect, this)
        && canvas_rect.pointInRect(x, y) && mAvatarPreview)
    {
        adjustCameraZoom(-static_cast<F32>(clicks) * 0.22f);
        return true;
    }
    return LLFloater::handleScrollWheel(x, y, clicks);
}

void FSForMeFittingRoom::drawAvatarPreview()
{
    if (!mAvatarPreview || !mAvatarPreview->hasGLTexture())
        return;
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect rect;
    if (!canvas || !canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this))
        return;
    LLGLSUIDefault ui_state;
    gGL.getTexUnit(0)->bind(mAvatarPreview);
    gGL.color4f(1.f, 1.f, 1.f, 1.f);
    gGL.begin(LLRender::TRIANGLES);
    gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(rect.mLeft + 1, rect.mTop - 1);
    gGL.texCoord2f(0.f, 0.f); gGL.vertex2i(rect.mLeft + 1, rect.mBottom + 1);
    gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(rect.mRight - 1, rect.mBottom + 1);
    gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(rect.mRight - 1, rect.mBottom + 1);
    gGL.texCoord2f(1.f, 1.f); gGL.vertex2i(rect.mRight - 1, rect.mTop - 1);
    gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(rect.mLeft + 1, rect.mTop - 1);
    gGL.end();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}
void FSForMeFittingRoom::changed(U32) { mDirty = true; }
void FSForMeFittingRoom::draw()
{
    if (mDirty) rebuild();
    LLFloater::draw();
    if (!isMinimized() && mAvatarPreview && mAvatarPreview->needsRender()) drawAvatarPreview();
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
        const auto suffix = std::to_string(i);
        const auto icon_name = LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags());
        getChild<LLThumbnailCtrl>("worn_thumbnail_" + suffix)->setValue(
            item->getThumbnailUUID().notNull() ? LLSD(item->getThumbnailUUID()) : LLSD(icon_name));
        getChild<LLTextBox>("worn_name_" + suffix)->setText(item->getName());
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
