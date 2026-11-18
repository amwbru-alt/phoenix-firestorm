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
#include "llrender.h"
#include "pipeline.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llfloatersnapshot.h"
#include "llimagepng.h"
#include "llviewerwindow.h"
#include "llviewertexture.h"
#include "llfile.h"
#include "lldate.h"
#include "lldir.h"
#include "llsdserialize.h"
#include <sstream>
#include <cstdio>

#include "llinventorymodel.h"
#include "llinventorytype.h"
#include "llwearabletype.h"
#include "lllineeditor.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "llthumbnailctrl.h"
#include "llinventoryicon.h"
#include "llviewerinventory.h"
#include "llstring.h"
#include <set>
#include <algorithm>
class FSForMeAvatarPreview final : public LLViewerDynamicTexture
{
public:
    FSForMeAvatarPreview()
        : LLViewerDynamicTexture(512, 307, 3, ORDER_MIDDLE, false)
    {
    }

    ~FSForMeAvatarPreview() override = default;

    bool needsRender() override { return mActive; }
    void setActive(bool active) { mActive = active; }
    void setYaw(F32 yaw) { mYaw = yaw; }
    void rotate(F32 delta) { mYaw += delta; }
    void tilt(F32 delta) { mElevation = llclamp(mElevation + delta, -0.35f, 0.65f); }
    void adjustDistance(F32 delta) { mDistance = llclamp(mDistance + delta, 1.8f, 7.0f); }
    void resetView() { mYaw = 0.f; mDistance = 2.7f; mElevation = 0.18f; }
    void requestCapture() { mCaptureRequested = true; }
    F32 getYaw() const { return mYaw; }
    F32 getDistance() const { return mDistance; }
    F32 getElevation() const { return mElevation; }
    LLPointer<LLImageRaw> takeCapturedImage()
    {
        LLPointer<LLImageRaw> image = mCapturedImage;
        mCapturedImage = nullptr;
        return image;
    }

    bool render() override
    {
        LLVOAvatar* avatar = gAgentAvatarp;
        if (!mActive || !avatar || avatar->isDead() || avatar->mDrawable.isNull())
            return false;

        LLViewerCamera* camera = LLViewerCamera::getInstance();
        camera->setAspect(static_cast<F32>(getWidth()) / static_cast<F32>(getHeight()));
        camera->setViewNoBroadcast(0.78539816f); // 45-degree preview FOV keeps the full avatar legible.

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
        // geometry, and renders attached objects supported by the preview into the currently bound
        // dynamic-texture target. The dynamic-texture lifecycle restores the
        // world camera and viewport after this call.
        gPipeline.generateImpostor(avatar, true, false);
        gGL.flush();

        // Capture the exact framebuffer used by the isolated ForMe preview.
        // This runs inside the dynamic-texture render target, before it is unbound.
        if (mCaptureRequested)
        {
            LLPointer<LLImageRaw> image = new LLImageRaw(getWidth(), getHeight(), 4);
            GLint previous_pack_alignment = 4;
            glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, getWidth(), getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image->getData());
            glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);
            image->verticalFlip();
            mCapturedImage = image;
            mCaptureRequested = false;
        }
        return true;
    }

private:
    bool mActive = false;
    bool mCaptureRequested = false;
    LLPointer<LLImageRaw> mCapturedImage;
    F32 mYaw = 0.f;
    F32 mDistance = 2.7f;
    F32 mElevation = 0.18f;
};

namespace
{
constexpr S32 WORN_ITEM_SLOTS = 16;
constexpr S32 LOOKBOOK_THUMBNAIL_SLOTS = 4;

std::string formeLookbookDirectory()
{
    return gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter() + "ForMeLookbook";
}

std::string formeLookbookMetadataPath()
{
    return formeLookbookDirectory() + gDirUtilp->getDirDelimiter() + "lookbook.xml";
}

LLSD loadFormeLookbook()
{
    LLSD entries = LLSD::emptyArray();
    const std::string contents = LLFile::getContents(formeLookbookMetadataPath());
    if (contents.empty()) return entries;
    std::istringstream input(contents);
    LLSD parsed;
    if (LLSDSerialize::fromXML(parsed, input) > 0 && parsed.isArray()) return parsed;
    LL_WARNS("ForMeLookbook") << "Could not parse lookbook metadata; preserving the existing file." << LL_ENDL;
    return entries;
}

bool saveFormeLookbook(const LLSD& entries)
{
    if (LLFile::mkdir(formeLookbookDirectory()) != 0) return false;
    std::ostringstream output;
    if (LLSDSerialize::toPrettyXML(entries, output) <= 0) return false;
    const std::string serialized = output.str();
    LLFILE* file = LLFile::fopen(formeLookbookMetadataPath(), "wb");
    if (!file) return false;
    const size_t written = fwrite(serialized.data(), 1, serialized.size(), file);
    LLFile::close(file);
    return written == serialized.size();
}

std::string fittingItemType(const LLViewerInventoryItem* item)
{
    if (!item) return "Предмет";
    if (item->getType() == LLAssetType::AT_CLOTHING)
    {
        switch (item->getWearableType())
        {
            case LLWearableType::WT_SHIRT: return "Верхняя одежда";
            case LLWearableType::WT_PANTS: return "Брюки";
            case LLWearableType::WT_SHOES: return "Обувь";
            case LLWearableType::WT_SOCKS: return "Носки";
            case LLWearableType::WT_JACKET: return "Куртка";
            case LLWearableType::WT_GLOVES: return "Перчатки";
            case LLWearableType::WT_UNDERSHIRT: return "Нижний слой";
            case LLWearableType::WT_UNDERPANTS: return "Нижнее бельё";
            case LLWearableType::WT_SKIRT: return "Юбка";
            case LLWearableType::WT_ALPHA: return "Alpha / скрывающий слой";
            case LLWearableType::WT_TATTOO: return "Татуировка";
            case LLWearableType::WT_PHYSICS: return "Физика";
            case LLWearableType::WT_UNIVERSAL: return "Универсальная одежда";
            default: return "Одежда";
        }
    }
    if (item->getType() == LLAssetType::AT_BODYPART)
    {
        switch (item->getWearableType())
        {
            case LLWearableType::WT_SHAPE: return "Форма тела";
            case LLWearableType::WT_SKIN: return "Кожа";
            case LLWearableType::WT_HAIR: return "Волосы";
            case LLWearableType::WT_EYES: return "Глаза";
            default: return "Часть тела";
        }
    }
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
    for (const auto& category : {std::make_pair("worn_filter_all", "all"),
                                 std::make_pair("worn_filter_clothing", "clothing"),
                                 std::make_pair("worn_filter_shape", "shape"),
                                 std::make_pair("worn_filter_skin", "skin"),
                                 std::make_pair("worn_filter_hair", "hair"),
                                 std::make_pair("worn_filter_eyes", "eyes"),
                                 std::make_pair("worn_filter_accessories", "accessories")})
        getChild<LLButton>(category.first)->setClickedCallback([this, value = std::string(category.second)](LLUICtrl*, const LLSD&) {
            setCategory(value);
        });
    getChild<LLButton>("fitting_refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { mDirty = true; });
    getChild<LLLineEditor>("saved_outfit_filter")->setKeystrokeCallback(
        [this](LLLineEditor*, void*) { mDirty = true; }, nullptr);
    getChild<LLButton>("saved_outfit_filter_clear")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        getChild<LLLineEditor>("saved_outfit_filter")->setText("");
        mDirty = true;
    });
    getChild<LLButton>("worn_page_previous")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mWornPage > 0) { --mWornPage; mDirty = true; }
    });
    getChild<LLButton>("worn_page_next")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        const S32 page_count = (static_cast<S32>(mWornItems.size()) + WORN_ITEM_SLOTS - 1) / WORN_ITEM_SLOTS;
        if (mWornPage + 1 < page_count) { ++mWornPage; mDirty = true; }
    });
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
    for (S32 i = 0; i < LOOKBOOK_THUMBNAIL_SLOTS; ++i)
        getChild<LLButton>("lookbook_thumb_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            if (i >= static_cast<S32>(mLookbookIds.size()) || mLookbookIds[i].empty()) return;
            selectLookbookPhoto(mLookbookIds[i]);
        });
    getChild<LLButton>("lookbook_prev")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mLookbookPage > 0) { --mLookbookPage; rebuildLookbook(); }
    });
    getChild<LLButton>("lookbook_next")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        const S32 pages = std::max(1, (static_cast<S32>(mFilteredLookbookEntries.size()) + LOOKBOOK_THUMBNAIL_SLOTS - 1) / LOOKBOOK_THUMBNAIL_SLOTS);
        if (mLookbookPage + 1 < pages) { ++mLookbookPage; rebuildLookbook(); }
    });
    getChild<LLButton>("lookbook_view_avatar")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mLookbookPreviewMode = false;
        mLookbookCompareMode = false;
    });
    LLComboBox* photo_category = getChild<LLComboBox>("lookbook_category");
    for (const auto& category : {std::make_pair("Everyday", "Everyday"),
                                 std::make_pair("Casual", "Casual"),
                                 std::make_pair("Formal", "Formal"),
                                 std::make_pair("Party", "Party"),
                                 std::make_pair("Fantasy", "Fantasy")})
        photo_category->add(category.first, LLSD(category.second));
    photo_category->setValue(LLSD("Everyday"));
    LLComboBox* photo_filter = getChild<LLComboBox>("lookbook_filter_category");
    photo_filter->add("Все категории", LLSD("all"));
    for (const auto& category : {std::make_pair("Everyday", "Everyday"),
                                 std::make_pair("Casual", "Casual"),
                                 std::make_pair("Formal", "Formal"),
                                 std::make_pair("Party", "Party"),
                                 std::make_pair("Fantasy", "Fantasy")})
        photo_filter->add(category.first, LLSD(category.second));
    photo_filter->setValue(LLSD("all"));
    photo_filter->setCommitCallback([this](LLUICtrl*, const LLSD&) {
        mLookbookFilterCategory = getChild<LLComboBox>("lookbook_filter_category")->getValue().asString();
        mLookbookPage = 0;
        rebuildLookbook();
    });
    getChild<LLButton>("lookbook_fullscreen")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        setLookbookFullscreenMode(!mLookbookFullscreenMode);
    });
    getChild<LLButton>("lookbook_sort_order")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mLookbookOldestFirst = !mLookbookOldestFirst;
        getChild<LLButton>("lookbook_sort_order")->setLabel(
            mLookbookOldestFirst ? "Сначала новые" : "Сначала старые");
        mLookbookPage = 0;
        rebuildLookbook();
    });
    getChild<LLButton>("lookbook_favorites_only")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mLookbookFavoritesOnly = !mLookbookFavoritesOnly;
        mLookbookPage = 0;
        getChild<LLButton>("lookbook_favorites_only")->setLabel(
            mLookbookFavoritesOnly ? "★ Избранное" : "☆ Все фото");
        rebuildLookbook();
    });
    getChild<LLButton>("lookbook_wear_outfit")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mSelectedLookbookId.empty()) return;
        const LLSD entries = loadFormeLookbook();
        for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
        {
            if ((*it)["id"].asString() != mSelectedLookbookId) continue;
            LLUUID outfit_id;
            outfit_id.set((*it)["outfit_id"].asString());
            const LLUUID outfits_root = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
            LLViewerInventoryCategory* outfit = outfit_id.isNull() ? nullptr : gInventory.getCategory(outfit_id);
            if (!outfit || outfit->getParentUUID() != outfits_root)
            {
                getChild<LLTextBox>("fitting_status")->setText(std::string("Связанный комплект больше не доступен в «Мои образы».");
                return;
            }
            LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(outfit, false);
            getChild<LLTextBox>("fitting_status")->setText(std::string("Применяем комплект «" + outfit->getName() + "»…");
            return;
        }
    });
    getChild<LLButton>("lookbook_toggle_favorite")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mSelectedLookbookId.empty()) return;
        LLSD entries = loadFormeLookbook();
        for (LLSD::array_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
        {
            if ((*it)["id"].asString() != mSelectedLookbookId) continue;
            (*it)["favorite"] = !(*it)["favorite"].asBoolean();
            if (saveFormeLookbook(entries))
            {
                getChild<LLButton>("lookbook_toggle_favorite")->setLabel(
                    (*it)["favorite"].asBoolean() ? "★ В избранном" : "☆ В избранное");
                rebuildLookbook();
            }
            else getChild<LLTextBox>("fitting_status")->setText(std::string("Не удалось сохранить избранное.");
            return;
        }
    });
    getChild<LLButton>("lookbook_save_metadata")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        saveSelectedLookbookMetadata();
    });
    getChild<LLButton>("lookbook_compare_a")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        assignLookbookCompareSlot(true);
    });
    getChild<LLButton>("lookbook_compare_b")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        assignLookbookCompareSlot(false);
    });
    getChild<LLButton>("lookbook_compare")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mLookbookCompareAId.empty() || mLookbookCompareBId.empty()
            || mLookbookCompareAId == mLookbookCompareBId)
        {
            getChild<LLTextBox>("fitting_status")->setText(std::string("Выберите два разных снимка: назначьте A и B.");
            return;
        }
        mLookbookPreviewMode = false;
        mLookbookCompareMode = true;
        getChild<LLTextBox>("fitting_status")->setText(std::string("Сравнение фотографий A / B.");
    });
    getChild<LLButton>("lookbook_delete")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mSelectedLookbookId.empty()) return;
        const std::string selected_id = mSelectedLookbookId;
        const LLSD entries = loadFormeLookbook();
        LLSD updated = LLSD::emptyArray();
        bool removed = false;
        for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
        {
            if ((*it)["id"].asString() == selected_id)
            {
                LLFile::remove((*it)["file"].asString());
                removed = true;
            }
            else updated.append(*it);
        }
        if (removed && saveFormeLookbook(updated))
        {
            mSelectedLookbookId.clear();
            mLookbookPreviewTexture = nullptr;
            mLookbookPreviewMode = false;
            getChild<LLButton>("lookbook_fullscreen")->setEnabled(false);
            getChild<LLTextBox>("lookbook_history_date")->setText(std::string("Дата снимка: —");
            mLookbookCompareMode = false;
            if (mLookbookCompareAId == selected_id) { mLookbookCompareAId.clear(); mLookbookCompareATexture = nullptr; }
            if (mLookbookCompareBId == selected_id) { mLookbookCompareBId.clear(); mLookbookCompareBTexture = nullptr; }
            getChild<LLButton>("lookbook_save_metadata")->setEnabled(false);
            getChild<LLButton>("lookbook_compare_a")->setEnabled(false);
            getChild<LLButton>("lookbook_compare_b")->setEnabled(false);
            rebuildLookbook();
            getChild<LLTextBox>("fitting_status")->setText(std::string("Фотография удалена из ForMe Lookbook.");
        }
        else getChild<LLTextBox>("fitting_status")->setText(std::string("Не удалось удалить фотографию или обновить каталог.");
    });
    getChild<LLButton>("fitting_capture_preview")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (!mAvatarPreview || !gAgentAvatarp || gAgentAvatarp->isDead() || gAgentAvatarp->mDrawable.isNull())
        {
            getChild<LLTextBox>("fitting_status")->setText(std::string("Невозможно снять preview: аватар ещё не готов.");
            return;
        }
        mAvatarPreview->requestCapture();
        getChild<LLTextBox>("fitting_status")->setText(std::string("Подготовка снимка 3D-примерочной…");
    });
    getChild<LLButton>("fitting_open_photo_studio")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        LLFloaterSnapshot* snapshot = LLFloaterReg::getTypedInstance<LLFloaterSnapshot>("snapshot");
        if (snapshot)
        {
            getChild<LLTextBox>("fitting_status")->setText(
                std::string("Открыта штатная фотостудия ForMe.World для дополнительных снимков сцены viewer. Для изолированного аватара используйте кнопку захвата preview.");
        }
    });
    getChild<LLButton>("fitting_open_inventory")->setClickedCallback([](LLUICtrl*, const LLSD&) {
        LLFloaterReg::showInstance("forme_inventory");
    });
    getChild<LLButton>("fitting_save_outfit")->setClickedCallback([this](LLUICtrl*, const LLSD&) { saveOutfit(); });
    getChild<LLButton>("fitting_compare_outfits")->setClickedCallback([this](LLUICtrl*, const LLSD&) { compareOutfits(); });
    getChild<LLComboBox>("compare_outfit_a")->setCommitCallback([this](LLUICtrl*, const LLSD&) { compareOutfits(); });
    getChild<LLComboBox>("compare_outfit_b")->setCommitCallback([this](LLUICtrl*, const LLSD&) { compareOutfits(); });
    getChild<LLButton>("fitting_move_saved_to_trash")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        const LLUUID outfit_id = getChild<LLComboBox>("saved_outfits")->getSelectedValue().asUUID();
        LLViewerInventoryCategory* outfit = gInventory.getCategory(outfit_id);
        const LLUUID outfits_root = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
        if (!outfit || outfit->getParentUUID() != outfits_root)
        {
            getChild<LLTextBox>("fitting_status")->setText(std::string("Выбранный образ больше не находится в папке «Мои образы».");
            mDirty = true;
            return;
        }
        gInventory.removeCategory(outfit_id); // Moves the outfit to Trash; it is recoverable.
        getChild<LLTextBox>("fitting_status")->setText(std::string("Образ перемещён в Корзину. Его можно восстановить из инвентаря.");
        mDirty = true;
    });
    getChild<LLButton>("fitting_wear_saved")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        const LLUUID outfit_id = getChild<LLComboBox>("saved_outfits")->getSelectedValue().asUUID();
        const LLUUID outfits_root = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
        LLViewerInventoryCategory* outfit = gInventory.getCategory(outfit_id);
        if (!outfit || outfit->getParentUUID() != outfits_root)
        {
            getChild<LLTextBox>("fitting_status")->setText(std::string("Выбранный образ недоступен в папке «Мои образы».");
            mDirty = true;
            return;
        }
        LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(outfit, false);
        getChild<LLTextBox>("fitting_status")->setText(std::string("Применяем образ «" + outfit->getName() + "»…");
    });
    for (S32 i = 0; i < WORN_ITEM_SLOTS; ++i)
        getChild<LLButton>("worn_remove_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            removeWornItem(i);
        });
    rebuildLookbook();
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
    setLookbookFullscreenMode(false);
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
    if (mLookbookFullscreenMode && (mask & MASK_LEFT))
    {
        const S32 edge = getLocalRect().getWidth() / 5;
        if (x < edge)
        {
            navigateLookbookPhoto(-1);
            return true;
        }
        if (x > getLocalRect().getWidth() - edge)
        {
            navigateLookbookPhoto(1);
            return true;
        }
    }
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
void FSForMeFittingRoom::changed(U32) { mDetailsOutfit.setNull(); mDirty = true; }

void FSForMeFittingRoom::setCategory(const std::string& category)
{
    if (mCategory == category) return;
    mCategory = category;
    mWornPage = 0;
    mDirty = true;
}

bool FSForMeFittingRoom::matchesCategory(const LLViewerInventoryItem* item) const
{
    if (!item || mCategory == "all") return item != nullptr;
    if (mCategory == "clothing") return item->getType() == LLAssetType::AT_CLOTHING;
    if (mCategory == "shape") return item->getType() == LLAssetType::AT_BODYPART && item->getWearableType() == LLWearableType::WT_SHAPE;
    if (mCategory == "skin") return item->getType() == LLAssetType::AT_BODYPART && item->getWearableType() == LLWearableType::WT_SKIN;
    if (mCategory == "hair") return item->getType() == LLAssetType::AT_BODYPART && item->getWearableType() == LLWearableType::WT_HAIR;
    if (mCategory == "eyes") return item->getType() == LLAssetType::AT_BODYPART && item->getWearableType() == LLWearableType::WT_EYES;
    if (mCategory == "accessories")
        return item->getType() == LLAssetType::AT_OBJECT;
    return true;
}
void FSForMeFittingRoom::draw()
{
    if (mAvatarPreview)
    {
        LLPointer<LLImageRaw> captured = mAvatarPreview->takeCapturedImage();
        if (captured.notNull())
        {
            LLPointer<LLImagePNG> png = new LLImagePNG();
            LLUUID photo_id;
            photo_id.generate();
            const std::string image_path = formeLookbookDirectory() + gDirUtilp->getDirDelimiter() + photo_id.asString() + ".png";
            LLUUID outfit_id = getChild<LLComboBox>("saved_outfits")->getSelectedValue().asUUID();
            LLViewerInventoryCategory* outfit = outfit_id.isNull() ? nullptr : gInventory.getCategory(outfit_id);
            const std::string outfit_name = outfit ? outfit->getName() : "Current outfit";
            if (png->encode(captured, 0.f) && LLFile::mkdir(formeLookbookDirectory()) == 0 && png->save(image_path))
            {
                LLSD entries = loadFormeLookbook();
                LLSD entry;
                entry["id"] = photo_id.asString();
                entry["file"] = image_path;
                entry["outfit_id"] = outfit_id.asString();
                entry["outfit_name"] = outfit_name;
                entry["title"] = outfit_name;
                entry["category"] = "Everyday";
                entry["favorite"] = false;
                entry["notes"] = "";
                entry["captured_at"] = LLDate::now().asString();
                entry["width"] = captured->getWidth();
                entry["height"] = captured->getHeight();
                entry["worn_item_count"] = static_cast<S32>(mWornItems.size());
                entry["camera_yaw"] = mAvatarPreview->getYaw();
                entry["camera_distance"] = mAvatarPreview->getDistance();
                entry["camera_elevation"] = mAvatarPreview->getElevation();
                entries.append(entry);
                if (saveFormeLookbook(entries))
                {
                    rebuildLookbook();
                    getChild<LLTextBox>("fitting_status")->setText(std::string("Фото сохранено в ForMe Lookbook: " + image_path);
                }
                else
                {
                    LLFile::remove(image_path);
                    getChild<LLTextBox>("fitting_status")->setText(std::string("PNG создан, но каталог Lookbook не удалось сохранить.");
                }
            }
            else
            {
                getChild<LLTextBox>("fitting_status")->setText(std::string("Не удалось сохранить PNG в папку ForMe Lookbook.");
            }
        }
    }
    if (mDirty) rebuild();
    updateSelectedOutfitDetails();
    if (mAvatarPreview) mAvatarPreview->setActive(!isMinimized());
    if (LLView* loading = getChild<LLView>("avatar_preview_loading"))
    {
        loading->setVisible(!mAvatarPreview || !mAvatarPreview->hasGLTexture());
    }
    LLFloater::draw();
    drawLookbookThumbnails();
    if (mLookbookCompareMode) drawLookbookComparison();
    else if (mLookbookPreviewMode) drawLookbookPhotoPreview();
    else if (mAvatarPreview && mAvatarPreview->needsRender()) drawAvatarPreview();
}
void FSForMeFittingRoom::selectLookbookPhoto(const std::string& id)
{
    const LLSD entries = loadFormeLookbook();
    for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
    {
        if ((*it)["id"].asString() != id) continue;
        mSelectedLookbookId = id;
        mLookbookCompareMode = false;
        mLookbookPreviewWidth = std::max(1, (*it)["width"].asInteger() > 0 ? (*it)["width"].asInteger() : 512);
        mLookbookPreviewHeight = std::max(1, (*it)["height"].asInteger() > 0 ? (*it)["height"].asInteger() : 307);
        getChild<LLTextBox>("lookbook_history_date")->setText(std::string("Снимок: " + (*it)["captured_at"].asString());
        getChild<LLLineEditor>("lookbook_title")->setText(
            (*it)["title"].asString().empty() ? (*it)["outfit_name"].asString() : (*it)["title"].asString());
        getChild<LLLineEditor>("lookbook_notes")->setText((*it)["notes"].asString());
        const std::string category = (*it)["category"].asString().empty() ? "Everyday" : (*it)["category"].asString();
        getChild<LLComboBox>("lookbook_category")->setValue(LLSD(category));
        getChild<LLButton>("lookbook_save_metadata")->setEnabled(true);
        getChild<LLButton>("lookbook_toggle_favorite")->setEnabled(true);
        getChild<LLButton>("lookbook_fullscreen")->setEnabled(true);
        LLUUID outfit_id;
        outfit_id.set((*it)["outfit_id"].asString());
        const LLUUID outfits_root = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
        LLViewerInventoryCategory* outfit = outfit_id.isNull() ? nullptr : gInventory.getCategory(outfit_id);
        getChild<LLButton>("lookbook_wear_outfit")->setEnabled(outfit && outfit->getParentUUID() == outfits_root);
        getChild<LLButton>("lookbook_toggle_favorite")->setLabel(
            (*it)["favorite"].asBoolean() ? "★ В избранном" : "☆ В избранное");
        getChild<LLButton>("lookbook_compare_a")->setEnabled(true);
        getChild<LLButton>("lookbook_compare_b")->setEnabled(true);
        const std::string path = (*it)["file"].asString();
        mLookbookPreviewTexture = LLViewerTextureManager::getFetchedTextureFromFile(
            path, FTT_LOCAL_FILE, true, LLGLTexture::BOOST_THUMBNAIL);
        if (mLookbookPreviewTexture)
        {
            mLookbookPreviewTexture->setKnownDrawSize(512, 307);
            mLookbookPreviewMode = true;
            getChild<LLTextBox>("fitting_status")->setText(
                (*it)["outfit_name"].asString() + " · " + (*it)["captured_at"].asString());
        }
        else
        {
            mLookbookPreviewMode = false;
            getChild<LLTextBox>("fitting_status")->setText(std::string("Не удалось загрузить выбранную фотографию.");
        }
        return;
    }
}

void FSForMeFittingRoom::setLookbookFullscreenMode(bool enabled)
{
    mLookbookFullscreenMode = enabled && mLookbookPreviewMode && mLookbookPreviewTexture.notNull();
    getChild<LLView>("avatar_stage")->setVisible(!mLookbookFullscreenMode);
    getChild<LLView>("wardrobe_surface")->setVisible(!mLookbookFullscreenMode);
    getChild<LLView>("save_surface")->setVisible(!mLookbookFullscreenMode);
    if (mLookbookFullscreenMode)
        getChild<LLTextBox>("fitting_status")->setText(std::string("Полноэкранный просмотр · ← / → — предыдущая и следующая фотография · Esc — закрыть");
}
void FSForMeFittingRoom::navigateLookbookPhoto(S32 delta)
{
    if (mSelectedLookbookId.empty() || mFilteredLookbookEntries.size() == 0) return;
    S32 selected_index = -1;
    for (S32 i = 0; i < static_cast<S32>(mFilteredLookbookEntries.size()); ++i)
    {
        if (mFilteredLookbookEntries[i]["id"].asString() == mSelectedLookbookId)
        {
            selected_index = i;
            break;
        }
    }
    if (selected_index < 0) return;
    const S32 display_step = mLookbookOldestFirst ? delta : -delta;
    const S32 next_index = llclamp(selected_index + display_step, 0, static_cast<S32>(mFilteredLookbookEntries.size()) - 1);
    if (next_index == selected_index) return;
    selectLookbookPhoto(mFilteredLookbookEntries[next_index]["id"].asString());
}
bool FSForMeFittingRoom::handleKeyHere(KEY key, MASK mask)
{
    if (mLookbookFullscreenMode)
    {
        if (key == KEY_ESCAPE)
        {
            setLookbookFullscreenMode(false);
            return true;
        }
        if (key == KEY_LEFT)
        {
            navigateLookbookPhoto(-1);
            return true;
        }
        if (key == KEY_RIGHT)
        {
            navigateLookbookPhoto(1);
            return true;
        }
    }
    return LLFloater::handleKeyHere(key, mask);
}

void FSForMeFittingRoom::saveSelectedLookbookMetadata()
{
    if (mSelectedLookbookId.empty())
    {
        getChild<LLTextBox>("fitting_status")->setText(std::string("Сначала выберите фотографию в Lookbook.");
        return;
    }
    LLSD entries = loadFormeLookbook();
    bool updated = false;
    for (LLSD::array_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
    {
        if ((*it)["id"].asString() != mSelectedLookbookId) continue;
        const std::string title = getChild<LLLineEditor>("lookbook_title")->getText();
        const std::string category = getChild<LLComboBox>("lookbook_category")->getValue().asString();
        (*it)["title"] = title.empty() ? (*it)["outfit_name"].asString() : title;
        (*it)["category"] = category.empty() ? "Everyday" : category;
        (*it)["notes"] = getChild<LLLineEditor>("lookbook_notes")->getText();
        updated = true;
        break;
    }
    if (updated && saveFormeLookbook(entries))
    {
        getChild<LLTextBox>("fitting_status")->setText(std::string("Сведения о фотографии сохранены.");
        rebuildLookbook();
    }
    else getChild<LLTextBox>("fitting_status")->setText(std::string("Не удалось сохранить сведения о фотографии.");
}

void FSForMeFittingRoom::assignLookbookCompareSlot(bool first)
{
    if (mSelectedLookbookId.empty()) return;
    const LLSD entries = loadFormeLookbook();
    for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
    {
        if ((*it)["id"].asString() != mSelectedLookbookId) continue;
        LLPointer<LLViewerFetchedTexture> texture = LLViewerTextureManager::getFetchedTextureFromFile(
            (*it)["file"].asString(), FTT_LOCAL_FILE, true, LLGLTexture::BOOST_THUMBNAIL);
        if (first)
        {
            mLookbookCompareAId = mSelectedLookbookId;
            mLookbookCompareATexture = texture;
        }
        else
        {
            mLookbookCompareBId = mSelectedLookbookId;
            mLookbookCompareBTexture = texture;
        }
        getChild<LLTextBox>("fitting_status")->setText(first ? "Снимок назначен в слот A." : "Снимок назначен в слот B.");
        return;
    }
}

void FSForMeFittingRoom::drawLookbookComparison()
{
    if (!mLookbookCompareMode || !mLookbookCompareATexture || !mLookbookCompareBTexture
        || !mLookbookCompareATexture->hasGLTexture() || !mLookbookCompareBTexture->hasGLTexture()) return;
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect rect;
    if (!canvas || !canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this)) return;
    LLGLSUIDefault ui_state;
    const S32 middle = rect.mLeft + rect.getWidth() / 2;
    const LLRect halves[] = {
        LLRect(rect.mLeft + 1, rect.mTop - 1, middle, rect.mBottom + 1),
        LLRect(middle, rect.mTop - 1, rect.mRight - 1, rect.mBottom + 1)
    };
    LLViewerFetchedTexture* textures[] = {mLookbookCompareATexture, mLookbookCompareBTexture};
    for (S32 i = 0; i < 2; ++i)
    {
        gGL.getTexUnit(0)->bind(textures[i]);
        gGL.color4f(1.f, 1.f, 1.f, 1.f);
        gGL.begin(LLRender::TRIANGLES);
        gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(halves[i].mLeft, halves[i].mTop);
        gGL.texCoord2f(0.f, 0.f); gGL.vertex2i(halves[i].mLeft, halves[i].mBottom);
        gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(halves[i].mRight, halves[i].mBottom);
        gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(halves[i].mRight, halves[i].mBottom);
        gGL.texCoord2f(1.f, 1.f); gGL.vertex2i(halves[i].mRight, halves[i].mTop);
        gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(halves[i].mLeft, halves[i].mTop);
        gGL.end();
    }
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void FSForMeFittingRoom::rebuildLookbook()
{
    const LLSD entries = loadFormeLookbook();
    mFilteredLookbookEntries = LLSD::emptyArray();
    for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
    {
        const std::string category = (*it)["category"].asString().empty() ? "Everyday" : (*it)["category"].asString();
        if (mLookbookFilterCategory != "all" && category != mLookbookFilterCategory) continue;
        if (mLookbookFavoritesOnly && !(*it)["favorite"].asBoolean()) continue;
        mFilteredLookbookEntries.append(*it);
    }
    const S32 count = static_cast<S32>(mFilteredLookbookEntries.size());
    const S32 page_count = std::max(1, (count + LOOKBOOK_THUMBNAIL_SLOTS - 1) / LOOKBOOK_THUMBNAIL_SLOTS);
    mLookbookPage = llclamp(mLookbookPage, 0, page_count - 1);
    mLookbookIds.assign(LOOKBOOK_THUMBNAIL_SLOTS, std::string());
    mLookbookTextures.clear();
    mLookbookTextures.resize(LOOKBOOK_THUMBNAIL_SLOTS);
    for (S32 slot = 0; slot < LOOKBOOK_THUMBNAIL_SLOTS; ++slot)
    {
        const S32 page_offset = mLookbookPage * LOOKBOOK_THUMBNAIL_SLOTS + slot;
        const S32 index = mLookbookOldestFirst ? page_offset : count - 1 - page_offset;
        LLButton* button = getChild<LLButton>("lookbook_thumb_" + std::to_string(slot));
        if (index < 0)
        {
            button->setEnabled(false);
            continue;
        }
        const LLSD& entry = mFilteredLookbookEntries[index];
        const std::string id = entry["id"].asString();
        mLookbookIds[slot] = id;
        button->setEnabled(true);
        mLookbookTextures[slot] = LLViewerTextureManager::getFetchedTextureFromFile(
            entry["file"].asString(), FTT_LOCAL_FILE, true, LLGLTexture::BOOST_THUMBNAIL);
        if (mLookbookTextures[slot])
            mLookbookTextures[slot]->setKnownDrawSize(128, 80);
    }
    getChild<LLButton>("lookbook_prev")->setEnabled(mLookbookPage > 0);
    getChild<LLButton>("lookbook_next")->setEnabled(mLookbookPage + 1 < page_count);
    getChild<LLTextBox>("lookbook_page_label")->setText(
        count == 0 ? "0 / 0" : std::to_string(mLookbookPage + 1) + " / " + std::to_string(page_count));
    getChild<LLButton>("lookbook_delete")->setEnabled(!mSelectedLookbookId.empty());
    getChild<LLButton>("lookbook_toggle_favorite")->setEnabled(!mSelectedLookbookId.empty());
    getChild<LLButton>("lookbook_wear_outfit")->setEnabled(false);
    if (!mSelectedLookbookId.empty())
    {
        bool found = false;
        for (LLSD::array_const_iterator it = entries.beginArray(); it != entries.endArray(); ++it)
            if ((*it)["id"].asString() == mSelectedLookbookId) { found = true; break; }
        if (!found)
        {
            mSelectedLookbookId.clear();
            mLookbookPreviewTexture = nullptr;
            mLookbookPreviewMode = false;
        }
    }
    if (count == 0)
        getChild<LLTextBox>("fitting_status")->setText(std::string("ForMe Lookbook пока пуст. Сделайте снимок 3D-примерочной.");
}

void FSForMeFittingRoom::drawLookbookThumbnails()
{
    LLGLSUIDefault ui_state;
    for (S32 i = 0; i < LOOKBOOK_THUMBNAIL_SLOTS; ++i)
    {
        if (i >= static_cast<S32>(mLookbookTextures.size()) || !mLookbookTextures[i]
            || !mLookbookTextures[i]->hasGLTexture()) continue;
        LLView* view = getChild<LLView>("lookbook_thumb_" + std::to_string(i));
        LLRect rect;
        if (!view || !view->localRectToOtherView(view->getLocalRect(), &rect, this)) continue;
        gGL.getTexUnit(0)->bind(mLookbookTextures[i]);
        gGL.color4f(1.f, 1.f, 1.f, 1.f);
        gGL.begin(LLRender::TRIANGLES);
        gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(rect.mLeft + 2, rect.mTop - 2);
        gGL.texCoord2f(0.f, 0.f); gGL.vertex2i(rect.mLeft + 2, rect.mBottom + 2);
        gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(rect.mRight - 2, rect.mBottom + 2);
        gGL.texCoord2f(1.f, 0.f); gGL.vertex2i(rect.mRight - 2, rect.mBottom + 2);
        gGL.texCoord2f(1.f, 1.f); gGL.vertex2i(rect.mRight - 2, rect.mTop - 2);
        gGL.texCoord2f(0.f, 1.f); gGL.vertex2i(rect.mLeft + 2, rect.mTop - 2);
        gGL.end();
    }
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void FSForMeFittingRoom::drawLookbookPhotoPreview()
{
    if (!mLookbookPreviewMode || !mLookbookPreviewTexture || !mLookbookPreviewTexture->hasGLTexture()) return;
    LLRect rect;
    if (mLookbookFullscreenMode)
    {
        rect = getLocalRect();
        rect.mTop -= getHeaderHeight();
    }
    else
    {
        LLView* canvas = getChild<LLView>("avatar_preview_canvas");
        if (!canvas || !canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this)) return;
    }
    if (mLookbookFullscreenMode)
    {
        const F32 source_aspect = static_cast<F32>(mLookbookPreviewWidth) / static_cast<F32>(mLookbookPreviewHeight);
        const F32 target_aspect = static_cast<F32>(rect.getWidth()) / static_cast<F32>(std::max(1, rect.getHeight()));
        if (source_aspect > target_aspect)
        {
            const S32 fitted_height = static_cast<S32>(rect.getWidth() / source_aspect);
            const S32 offset = (rect.getHeight() - fitted_height) / 2;
            rect.mBottom += offset;
            rect.mTop = rect.mBottom + fitted_height;
        }
        else
        {
            const S32 fitted_width = static_cast<S32>(rect.getHeight() * source_aspect);
            const S32 offset = (rect.getWidth() - fitted_width) / 2;
            rect.mLeft += offset;
            rect.mRight = rect.mLeft + fitted_width;
        }
    }
    LLGLSUIDefault ui_state;
    gGL.getTexUnit(0)->bind(mLookbookPreviewTexture);
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
    if (mLookbookFullscreenMode)
    {
        const S32 center_y = rect.mBottom + rect.getHeight() / 2;
        const S32 left_x = rect.mLeft + 42;
        const S32 right_x = rect.mRight - 42;
        gGL.color4f(1.f, 1.f, 1.f, 0.78f);
        gGL.begin(LLRender::TRIANGLES);
        gGL.vertex2i(left_x - 12, center_y);
        gGL.vertex2i(left_x + 8, center_y + 22);
        gGL.vertex2i(left_x + 8, center_y - 22);
        gGL.vertex2i(right_x + 12, center_y);
        gGL.vertex2i(right_x - 8, center_y - 22);
        gGL.vertex2i(right_x - 8, center_y + 22);
        gGL.end();
    }
}

void FSForMeFittingRoom::rebuild()
{
    mDirty = false;
    mWornItems.clear();
    auto* outfits_combo = getChild<LLComboBox>("saved_outfits");
    auto* compare_a = getChild<LLComboBox>("compare_outfit_a");
    auto* compare_b = getChild<LLComboBox>("compare_outfit_b");
    const LLUUID previous_outfit = outfits_combo->getSelectedValue().asUUID();
    const LLUUID previous_compare_a = compare_a->getSelectedValue().asUUID();
    const LLUUID previous_compare_b = compare_b->getSelectedValue().asUUID();
    outfits_combo->removeall();
    compare_a->removeall();
    compare_b->removeall();
    mSavedOutfitCount = 0;
    const LLUUID outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
    LLInventoryModel::cat_array_t* outfit_categories = nullptr;
    LLInventoryModel::item_array_t* outfit_items = nullptr;
    gInventory.getDirectDescendentsOf(outfits_id, outfit_categories, outfit_items);
    if (outfit_categories)
    {
        std::vector<LLViewerInventoryCategory*> sorted_outfits;
        for (const auto& outfit : *outfit_categories)
            if (outfit.notNull()) sorted_outfits.push_back(outfit);
        std::sort(sorted_outfits.begin(), sorted_outfits.end(),
            [](const LLViewerInventoryCategory* left, const LLViewerInventoryCategory* right)
            {
                return LLStringUtil::compareInsensitive(left->getName(), right->getName()) < 0;
            });
        std::string outfit_filter = utf8str_tolower(getChild<LLLineEditor>("saved_outfit_filter")->getValue().asString());
        for (const auto* outfit : sorted_outfits)
        {
            LLInventoryModel::cat_array_t* child_categories = nullptr;
            LLInventoryModel::item_array_t* child_items = nullptr;
            gInventory.getDirectDescendentsOf(outfit->getUUID(), child_categories, child_items);
            bool matches_filter = outfit_filter.empty()
                || utf8str_tolower(outfit->getName()).find(outfit_filter) != std::string::npos;
            if (!matches_filter && child_items)
            {
                for (const auto& item : *child_items)
                {
                    if (item.notNull()
                        && utf8str_tolower(item->getName()).find(outfit_filter) != std::string::npos)
                    {
                        matches_filter = true;
                        break;
                    }
                }
            }
            if (!matches_filter) continue;
            const S32 item_count = child_items ? static_cast<S32>(child_items->size()) : 0;
            const std::string label = outfit->getName() + " (" + std::to_string(item_count) + ")";
            outfits_combo->add(label, outfit->getUUID());
            compare_a->add(label, outfit->getUUID());
            compare_b->add(label, outfit->getUUID());
            ++mSavedOutfitCount;
        }
    }
    getChild<LLButton>("fitting_wear_saved")->setEnabled(mSavedOutfitCount > 0);
    getChild<LLButton>("fitting_move_saved_to_trash")->setEnabled(mSavedOutfitCount > 0);
    getChild<LLButton>("fitting_compare_outfits")->setEnabled(mSavedOutfitCount > 1);
    if (mSavedOutfitCount > 0)
    {
        if (!outfits_combo->setCurrentByID(previous_outfit)) outfits_combo->selectFirstItem();
        if (!compare_a->setCurrentByID(previous_compare_a)) compare_a->selectFirstItem();
        if (!compare_b->setCurrentByID(previous_compare_b))
        {
            compare_b->selectFirstItem();
            if (mSavedOutfitCount > 1) compare_b->selectNthItem(1);
        }
    }
    LLInventoryModel::cat_array_t* categories = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(LLAppearanceMgr::instance().getCOF(), categories, items);
    if (items)
        for (const auto& item : *items)
            if (item.notNull() && item->getIsLinkType() && matchesCategory(item)) mWornItems.push_back(item->getUUID());
    const S32 page_count = std::max(1, (static_cast<S32>(mWornItems.size()) + WORN_ITEM_SLOTS - 1) / WORN_ITEM_SLOTS);
    mWornPage = llclamp(mWornPage, 0, page_count - 1);
    const S32 page_start = mWornPage * WORN_ITEM_SLOTS;
    for (S32 i = 0; i < WORN_ITEM_SLOTS; ++i)
    {
        const S32 item_index = page_start + i;
        const bool present = item_index < static_cast<S32>(mWornItems.size());
        getChild<LLPanel>("worn_card_" + std::to_string(i))->setVisible(present);
        if (!present) continue;
        const auto* item = gInventory.getItem(mWornItems[item_index]);
        if (!item) continue;
        const auto suffix = std::to_string(i);
        const auto icon_name = LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags());
        getChild<LLThumbnailCtrl>("worn_thumbnail_" + suffix)->setValue(
            item->getThumbnailUUID().notNull() ? LLSD(item->getThumbnailUUID()) : LLSD(icon_name));
        getChild<LLTextBox>("worn_name_" + suffix)->setText(item->getName());
        getChild<LLTextBox>("worn_type_" + std::to_string(i))->setText(fittingItemType(item));
    }
    getChild<LLTextBox>("fitting_item_count")->setText(std::to_string(mWornItems.size()) + " предметов в категории");
    getChild<LLTextBox>("worn_page_label")->setText(std::to_string(mWornPage + 1) + " / " + std::to_string(page_count));
    getChild<LLButton>("worn_page_previous")->setEnabled(mWornPage > 0);
    getChild<LLButton>("worn_page_next")->setEnabled(mWornPage + 1 < page_count);
    getChild<LLButton>("worn_filter_all")->setToggleState(mCategory == "all");
    getChild<LLButton>("worn_filter_clothing")->setToggleState(mCategory == "clothing");
    getChild<LLButton>("worn_filter_shape")->setToggleState(mCategory == "shape");
    getChild<LLButton>("worn_filter_skin")->setToggleState(mCategory == "skin");
    getChild<LLButton>("worn_filter_hair")->setToggleState(mCategory == "hair");
    getChild<LLButton>("worn_filter_eyes")->setToggleState(mCategory == "eyes");
    getChild<LLButton>("worn_filter_accessories")->setToggleState(mCategory == "accessories");
    if (!outfit_filter.empty() && mSavedOutfitCount == 0)
        getChild<LLTextBox>("fitting_status")->setText(std::string("По запросу не найдено сохранённых образов.");
    else getChild<LLTextBox>("fitting_status")->setText(mWornItems.empty()
        ? "Ожидаем данные текущего образа. Если аватар ещё загружается, обновите список позже."
        : "Список синхронизирован с текущим образом аватара.");
}

void FSForMeFittingRoom::updateSelectedOutfitDetails()
{
    const LLUUID outfit_id = getChild<LLComboBox>("saved_outfits")->getSelectedValue().asUUID();
    if (outfit_id == mDetailsOutfit) return;
    mDetailsOutfit = outfit_id;
    if (outfit_id.isNull())
    {
        getChild<LLTextBox>("saved_outfit_details")->setText(std::string("Выберите сохранённый образ, чтобы посмотреть его состав.");
        return;
    }

    const LLViewerInventoryCategory* outfit = gInventory.getCategory(outfit_id);
    if (!outfit || outfit->getParentUUID() != gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS))
    {
        getChild<LLTextBox>("saved_outfit_details")->setText(std::string("Сведения об образе недоступны.");
        return;
    }

    LLInventoryModel::cat_array_t* categories = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(outfit_id, categories, items);
    S32 item_count = 0;
    std::vector<std::string> examples;
    if (items)
    {
        for (const auto& item : *items)
        {
            if (item.isNull()) continue;
            ++item_count;
            if (examples.size() < 2) examples.push_back(item->getName());
        }
    }

    std::string details = outfit->getName() + " · " + std::to_string(item_count) + " предметов";
    if (!examples.empty())
        details += " · " + examples.front() + (examples.size() > 1 ? ", " + examples.back() : "");
    getChild<LLTextBox>("saved_outfit_details")->setText(details);
}

void FSForMeFittingRoom::removeWornItem(S32 index)
{
    const S32 item_index = mWornPage * WORN_ITEM_SLOTS + index;
    if (index < 0 || index >= WORN_ITEM_SLOTS || item_index >= static_cast<S32>(mWornItems.size())) return;
    LLAppearanceMgr::instance().removeItemFromAvatar(mWornItems[item_index]);
    mDirty = true;
}

void FSForMeFittingRoom::compareOutfits()
{
    const LLUUID first_id = getChild<LLComboBox>("compare_outfit_a")->getSelectedValue().asUUID();
    const LLUUID second_id = getChild<LLComboBox>("compare_outfit_b")->getSelectedValue().asUUID();
    if (first_id.isNull() || second_id.isNull() || first_id == second_id)
    {
        getChild<LLTextBox>("compare_summary")->setText(std::string("Выберите два разных сохранённых образа.");
        getChild<LLTextBox>("compare_only_a")->setText(std::string("");
        getChild<LLTextBox>("compare_only_b")->setText(std::string("");
        getChild<LLTextBox>("fitting_status")->setText(std::string("Сравнение не выполнено.");
        return;
    }
    auto collect_items = [](const LLUUID& outfit_id, std::set<LLUUID>& item_ids)
    {
        LLInventoryModel::cat_array_t* categories = nullptr;
        LLInventoryModel::item_array_t* items = nullptr;
        gInventory.getDirectDescendentsOf(outfit_id, categories, items);
        if (!items) return;
        for (const auto& item : *items)
        {
            if (item.isNull()) continue;
            const LLUUID linked_id = item->getIsLinkType() ? item->getLinkedUUID() : item->getUUID();
            if (linked_id.notNull()) item_ids.insert(linked_id);
        }
    };
    std::set<LLUUID> first_items, second_items;
    collect_items(first_id, first_items);
    collect_items(second_id, second_items);
    if (first_items.empty() && second_items.empty())
    {
        getChild<LLTextBox>("compare_summary")->setText(std::string("В выбранных образах пока нет предметов для сравнения.");
        getChild<LLTextBox>("compare_only_a")->setText(std::string("");
        getChild<LLTextBox>("compare_only_b")->setText(std::string("");
        getChild<LLTextBox>("fitting_status")->setText(std::string("Сравнение не выполнено.");
        return;
    }
    S32 shared = 0;
    std::vector<std::string> only_first_names;
    std::vector<std::string> only_second_names;
    auto collect_names = [](const LLUUID& outfit_id, const std::set<LLUUID>& other_items,
                            std::vector<std::string>& names)
    {
        LLInventoryModel::cat_array_t* categories = nullptr;
        LLInventoryModel::item_array_t* items = nullptr;
        gInventory.getDirectDescendentsOf(outfit_id, categories, items);
        if (!items) return;
        for (const auto& item : *items)
        {
            if (item.isNull()) continue;
            const LLUUID linked_id = item->getIsLinkType() ? item->getLinkedUUID() : item->getUUID();
            if (linked_id.notNull() && !other_items.count(linked_id) && names.size() < 4)
                names.push_back(item->getName());
        }
    };
    for (const auto& item_id : first_items)
        if (second_items.count(item_id)) ++shared;
    collect_names(first_id, second_items, only_first_names);
    collect_names(second_id, first_items, only_second_names);
    const S32 only_first = static_cast<S32>(first_items.size()) - shared;
    const S32 only_second = static_cast<S32>(second_items.size()) - shared;
    const auto* first = gInventory.getCategory(first_id);
    const auto* second = gInventory.getCategory(second_id);
    const std::string first_name = first ? first->getName() : "Образ A";
    const std::string second_name = second ? second->getName() : "Образ B";
    auto format_names = [](const std::vector<std::string>& names, S32 total)
    {
        if (total == 0) return std::string("Нет уникальных предметов");
        std::string result;
        for (const auto& name : names)
        {
            if (!result.empty()) result += ", ";
            result += name;
        }
        const S32 omitted = total - static_cast<S32>(names.size());
        if (omitted > 0) result += " и ещё " + std::to_string(omitted);
        return result;
    };
    getChild<LLTextBox>("compare_summary")->setText(
        first_name + " / " + second_name + " · общих: " + std::to_string(shared)
        + " · уникальных в A: " + std::to_string(only_first)
        + " · уникальных в B: " + std::to_string(only_second));
    getChild<LLTextBox>("compare_only_a")->setText(std::string("Только в A: " + format_names(only_first_names, only_first));
    getChild<LLTextBox>("compare_only_b")->setText(std::string("Только в B: " + format_names(only_second_names, only_second));
    getChild<LLTextBox>("fitting_status")->setText(std::string("Сравнение готово: " + first_name + " и " + second_name + ".");
}

void FSForMeFittingRoom::saveOutfit()
{
    std::string name = getChild<LLLineEditor>("outfit_name")->getValue().asString();
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        getChild<LLTextBox>("fitting_status")->setText(std::string("Введите название комплекта перед сохранением.");
        return;
    }
    const auto last = name.find_last_not_of(" \t\r\n");
    name = name.substr(first, last - first + 1);
    const LLUUID outfits_root = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
    LLInventoryModel::cat_array_t* outfit_categories = nullptr;
    LLInventoryModel::item_array_t* outfit_items = nullptr;
    gInventory.getDirectDescendentsOf(outfits_root, outfit_categories, outfit_items);
    if (outfit_categories)
    {
        for (const auto& outfit : *outfit_categories)
        {
            if (outfit.notNull() && LLStringUtil::compareInsensitive(outfit->getName(), name) == 0)
            {
                getChild<LLTextBox>("fitting_status")->setText(std::string("Образ с таким названием уже существует. Выберите другое название.");
                return;
            }
        }
    }
    LLAppearanceMgr::instance().makeNewOutfitLinks(name, false);
    getChild<LLTextBox>("fitting_status")->setText(std::string("Создаём комплект «" + name + "» в папке «Мои образы»…");
}
