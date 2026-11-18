/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "lldynamictexture.h"
#include "llviewercamera.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llrender.h"
#include "pipeline.h"
#include "llfocusmgr.h"
#include "fsformeinventory.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lliconctrl.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventoryfunctions.h"
#include "llthumbnailctrl.h"
#include "llpanel.h"
#include "llinventorytype.h"
#include "llsearcheditor.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"
#include "llviewerinventory.h"
#include "llwearabletype.h"
#include "lldate.h"
#include <algorithm>
#include <cctype>
#include <set>

class FSForMeInventoryAvatarPreview final : public LLViewerDynamicTexture
{
public:
    FSForMeInventoryAvatarPreview() : LLViewerDynamicTexture(640, 760, 3, ORDER_MIDDLE, false) {}
    bool needsRender() override { return mActive; }
    void setActive(bool active) { mActive = active; }
    void setYaw(F32 yaw) { mYaw = yaw; }
    void rotate(F32 delta) { mYaw += delta; }
    void tilt(F32 delta) { mElevation = llclamp(mElevation + delta, -0.35f, 0.65f); }
    void zoom(F32 delta) { mDistance = llclamp(mDistance + delta, 1.8f, 7.0f); }
    void reset() { mYaw = 0.f; mDistance = 2.7f; mElevation = 0.18f; }
    bool render() override
    {
        LLVOAvatar* avatar = gAgentAvatarp;
        if (!mActive || !avatar || avatar->isDead() || avatar->mDrawable.isNull()) return false;
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        camera->setAspect(static_cast<F32>(getWidth()) / static_cast<F32>(getHeight()));
        camera->setViewNoBroadcast(0.78539816f);
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
        gPipeline.generateImpostor(avatar, true, false);
        gGL.flush();
        return true;
    }
private:
    bool mActive = false;
    F32 mYaw = 0.f;
    F32 mDistance = 2.7f;
    F32 mElevation = 0.18f;
};

namespace
{
constexpr S32 CARD_COUNT = 20;
std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
void collectItems(const LLUUID& folder, std::vector<LLUUID>& output, std::set<LLUUID>& visited)
{
    if (folder.isNull() || !visited.insert(folder).second) return;
    LLInventoryModel::cat_array_t* categories = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(folder, categories, items);
    if (items)
        for (const auto& item : *items) if (item.notNull()) output.push_back(item->getUUID());
    if (categories)
        for (const auto& category : *categories) if (category.notNull()) collectItems(category->getUUID(), output, visited);
}
}
FSForMeInventoryBrowser::FSForMeInventoryBrowser(const LLSD& key) : LLFloater(key) {}
FSForMeInventoryBrowser::~FSForMeInventoryBrowser() { gInventory.removeObserver(this); delete mAvatarPreview; }

bool FSForMeInventoryBrowser::postBuild()
{
    gInventory.addObserver(this);
    mAvatarPreview = new FSForMeInventoryAvatarPreview();
    getChild<LLSearchEditor>("query")->setCommitCallback([this](LLUICtrl*, const LLSD&) {
        mQuery = getChild<LLSearchEditor>("query")->getValue().asString();
        mPage = 0; mDirty = true;
    });
    getChild<LLButton>("avatar_reset")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->reset();
        getChild<LLButton>("camera_front")->setToggleState(true);
        getChild<LLButton>("camera_side")->setToggleState(false);
        getChild<LLButton>("camera_back")->setToggleState(false);
    });
    getChild<LLButton>("camera_front")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->setYaw(0.f);
        getChild<LLButton>("camera_front")->setToggleState(true);
        getChild<LLButton>("camera_side")->setToggleState(false);
        getChild<LLButton>("camera_back")->setToggleState(false);
    });
    getChild<LLButton>("camera_side")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->setYaw(1.5707963f);
        getChild<LLButton>("camera_front")->setToggleState(false);
        getChild<LLButton>("camera_side")->setToggleState(true);
        getChild<LLButton>("camera_back")->setToggleState(false);
    });
    getChild<LLButton>("camera_back")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->setYaw(3.1415926f);
        getChild<LLButton>("camera_front")->setToggleState(false);
        getChild<LLButton>("camera_side")->setToggleState(false);
        getChild<LLButton>("camera_back")->setToggleState(true);
    });
    getChild<LLButton>("camera_zoom_in")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->zoom(-0.25f);
    });
    getChild<LLButton>("camera_zoom_out")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (mAvatarPreview) mAvatarPreview->zoom(0.25f);
    });
    getChild<LLButton>("all_items_tab")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mCategory = "all"; mPage = 0; mDirty = true;
    });
    getChild<LLButton>("favorites_tab")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mCategory = "favorites"; mPage = 0; mDirty = true;
    });
    getChild<LLButton>("purchased_tab")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mCategory = "recent"; mPage = 0; mDirty = true;
    });
    getChild<LLButton>("refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { mDirty = true; });
    getChild<LLComboBox>("category_filter")->setCommitCallback([this](LLUICtrl*, const LLSD&) {
        mCategory = getChild<LLComboBox>("category_filter")->getValue().asString();
        mPage = 0;
        mDirty = true;
    });
    for (const auto& category : {std::make_pair("category_all", "all"), std::make_pair("category_clothing", "clothing"),
                                 std::make_pair("category_shoes", "shoes"), std::make_pair("category_hair", "hair"),
                                 std::make_pair("category_tattoos", "tattoos"), std::make_pair("category_jewelry", "jewelry"),
                                 std::make_pair("category_accessories", "accessories"), std::make_pair("category_objects", "objects"),
                                 std::make_pair("category_animations", "animations"), std::make_pair("category_favorites", "favorites"),
                                 std::make_pair("category_recent", "recent")})
        getChild<LLButton>(category.first)->setClickedCallback([this, value = std::string(category.second)](LLUICtrl*, const LLSD&) {
            mCategory = value; mPage = 0; mDirty = true;
        });
    for (S32 i = 0; i < CARD_COUNT; ++i)
        getChild<LLButton>("item_favorite_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            const S32 index = mPage * CARD_COUNT + i;
            if (index < static_cast<S32>(mVisibleItems.size())) toggle_favorite(mVisibleItems[index]);
        });
    getChild<LLButton>("previous")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if (mPage > 0) { --mPage; mDirty = true; } });
    getChild<LLButton>("next")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if ((mPage + 1) * CARD_COUNT < static_cast<S32>(mVisibleItems.size())) { ++mPage; mDirty = true; } });
    for (S32 i = 0; i < CARD_COUNT; ++i)
        getChild<LLButton>("item_select_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            const S32 index = mPage * CARD_COUNT + i;
            if (index < static_cast<S32>(mVisibleItems.size())) selectItem(mVisibleItems[index]);
        });
    getChild<LLButton>("detail_favorite")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if (mSelectedItem.notNull()) toggle_favorite(mSelectedItem); });
    getChild<LLButton>("apply")->setClickedCallback([this](LLUICtrl*, const LLSD&) { applySelected(); });
    getChild<LLButton>("open_folder")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (auto* item = gInventory.getItem(mSelectedItem)) { mFolder = item->getParentUUID(); mPage = 0; mDirty = true; }
    });
    getChild<LLButton>("folder_back")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        mFolder = gInventory.getRootFolderID(); mPage = 0; mDirty = true;
    });
    return true;
}
void FSForMeInventoryBrowser::onOpen(const LLSD&)
{
    if (mAvatarPreview)
    {
        mAvatarPreview->reset();
        mAvatarPreview->setActive(true);
    }
    getChild<LLButton>("camera_front")->setToggleState(true);
    getChild<LLButton>("camera_side")->setToggleState(false);
    getChild<LLButton>("camera_back")->setToggleState(false);
    mSelectedItem.setNull();
    getChild<LLTextBox>("detail_name")->setText("Выберите предмет");
    getChild<LLTextBox>("detail_type")->setText("");
    getChild<LLTextBox>("detail_folder")->setText("");
    getChild<LLTextBox>("detail_creator")->setText("");
    getChild<LLTextBox>("detail_created")->setText("");
    getChild<LLTextBox>("detail_description")->setText("Информация о выбранном предмете появится здесь.");
    getChild<LLButton>("apply")->setEnabled(false);
    getChild<LLButton>("open_folder")->setEnabled(false);
    getChild<LLButton>("detail_favorite")->setEnabled(false);
    mFolder = gInventory.getRootFolderID();
    mDirty = true;
}
void FSForMeInventoryBrowser::changed(U32)
{
    mDirty = true;
}
void FSForMeInventoryBrowser::onClose(bool)
{
    mDraggingPreview = false;
    if (hasMouseCapture()) gFocusMgr.setMouseCapture(nullptr);
    if (mAvatarPreview) mAvatarPreview->setActive(false);
}
bool FSForMeInventoryBrowser::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect rect;
    if ((mask & MASK_LEFT) && canvas && canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this)
        && rect.pointInRect(x, y) && mAvatarPreview)
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
bool FSForMeInventoryBrowser::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (mDraggingPreview)
    {
        mDraggingPreview = false;
        if (hasMouseCapture()) gFocusMgr.setMouseCapture(nullptr);
        return true;
    }
    return LLFloater::handleMouseUp(x, y, mask);
}
bool FSForMeInventoryBrowser::handleHover(S32 x, S32 y, MASK mask)
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
bool FSForMeInventoryBrowser::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect rect;
    if (canvas && canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this)
        && rect.pointInRect(x, y) && mAvatarPreview)
    {
        mAvatarPreview->zoom(-static_cast<F32>(clicks) * 0.22f);
        return true;
    }
    return LLFloater::handleScrollWheel(x, y, clicks);
}
void FSForMeInventoryBrowser::drawAvatarPreview()
{
    if (!mAvatarPreview || !mAvatarPreview->hasGLTexture()) return;
    LLView* canvas = getChild<LLView>("avatar_preview_canvas");
    LLRect rect;
    if (!canvas || !canvas->localRectToOtherView(canvas->getLocalRect(), &rect, this)) return;
    LLGLSUIDefault state;
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
void FSForMeInventoryBrowser::draw()
{
    if (mDirty) rebuild();
    if (mAvatarPreview) mAvatarPreview->setActive(!isMinimized());
    if (LLView* loading = getChild<LLView>("avatar_preview_loading"))
        loading->setVisible(!mAvatarPreview || !mAvatarPreview->hasGLTexture());
    LLFloater::draw();
    if (mAvatarPreview && mAvatarPreview->needsRender()) drawAvatarPreview();
}
bool FSForMeInventoryBrowser::matches(const LLViewerInventoryItem* item) const
{
    if (!item) return false;
    const auto type = item->getType();
    if (mCategory == "clothing" && type != LLAssetType::AT_CLOTHING && type != LLAssetType::AT_BODYPART) return false;
    if (mCategory == "shoes" && (type != LLAssetType::AT_CLOTHING || item->getWearableType() != LLWearableType::WT_SHOES)) return false;
    if (mCategory == "hair" && (type != LLAssetType::AT_BODYPART || item->getWearableType() != LLWearableType::WT_HAIR)) return false;
    if (mCategory == "tattoos" && (type != LLAssetType::AT_BODYPART || item->getWearableType() != LLWearableType::WT_SKIN)) return false;
    if (mCategory == "jewelry" && type != LLAssetType::AT_OBJECT) return false;
    if (mCategory == "accessories" && (type != LLAssetType::AT_OBJECT || item->getInventoryType() != LLInventoryType::IT_WEARABLE)) return false;
    if (mCategory == "objects" && (type != LLAssetType::AT_OBJECT || item->getInventoryType() == LLInventoryType::IT_WEARABLE)) return false;
    if (mCategory == "animations" && type != LLAssetType::AT_ANIMATION && type != LLAssetType::AT_GESTURE) return false;
    if (mCategory == "favorites" && !get_is_favorite(item->getUUID())) return false;
    if (mCategory == "recent" && std::find(mRecentItems.begin(), mRecentItems.end(), item->getUUID()) == mRecentItems.end()) return false;
    return mQuery.empty() || lower(item->getName()).find(lower(mQuery)) != std::string::npos;
}
void FSForMeInventoryBrowser::rebuild()
{
    mDirty = false;
    std::vector<LLUUID> all;
    std::set<LLUUID> visited;
    collectItems(mFolder.isNull() ? gInventory.getRootFolderID() : mFolder, all, visited);
    mVisibleItems.clear();
    if (mCategory == "recent")
    {
        for (const auto& id : mRecentItems) if (matches(gInventory.getItem(id))) mVisibleItems.push_back(id);
    }
    else
    {
        std::sort(all.begin(), all.end(), [](const LLUUID& a, const LLUUID& b) {
            const auto* left = gInventory.getItem(a); const auto* right = gInventory.getItem(b);
            return left && right ? lower(left->getName()) < lower(right->getName()) : a.asString() < b.asString();
        });
        for (const auto& id : all) if (matches(gInventory.getItem(id))) mVisibleItems.push_back(id);
    }
    if (mSelectedItem.notNull() && std::find(mVisibleItems.begin(), mVisibleItems.end(), mSelectedItem) == mVisibleItems.end())
    {
        mSelectedItem.setNull();
        getChild<LLTextBox>("detail_name")->setText("Выберите предмет");
        getChild<LLTextBox>("detail_type")->setText("");
        getChild<LLTextBox>("detail_folder")->setText("");
        getChild<LLTextBox>("detail_creator")->setText("");
        getChild<LLTextBox>("detail_created")->setText("");
        getChild<LLTextBox>("detail_description")->setText("Информация о выбранном предмете появится здесь.");
        getChild<LLButton>("apply")->setEnabled(false);
        getChild<LLButton>("open_folder")->setEnabled(false);
        getChild<LLButton>("detail_favorite")->setEnabled(false);
    }
    const S32 pages = llmax(1, (static_cast<S32>(mVisibleItems.size()) + CARD_COUNT - 1) / CARD_COUNT);
    mPage = llclamp(mPage, 0, pages - 1);
    auto* grid = getChild<LLPanel>("items_grid");
    for (S32 i = 0; i < CARD_COUNT; ++i)
    {
        const auto suffix = std::to_string(i);
        const S32 index = mPage * CARD_COUNT + i;
        const bool present = index < static_cast<S32>(mVisibleItems.size());
        getChild<LLPanel>("item_card_" + suffix)->setVisible(present);
        if (!present) continue;
        auto* item = gInventory.getItem(mVisibleItems[index]);
        if (!item) continue;
        const auto icon_name = LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags());
        getChild<LLThumbnailCtrl>("item_thumbnail_" + suffix)->setValue(item->getThumbnailUUID().notNull() ? LLSD(item->getThumbnailUUID()) : LLSD(icon_name));
        LLButton* favorite_button = getChild<LLButton>("item_favorite_" + suffix);
        favorite_button->setLabel(get_is_favorite(item->getUUID()) ? "★" : "☆");
        favorite_button->setToggleState(get_is_favorite(item->getUUID()));
        getChild<LLTextBox>("item_name_" + suffix)->setText(item->getName());
        LLButton* select_button = getChild<LLButton>("item_select_" + suffix);
        const bool selected = mSelectedItem == item->getUUID();
        select_button->setLabel(selected ? "✦ Выбрано" : "Выбрать");
        select_button->setToggleState(selected);
    }
    grid->reshape(644, 610);
    getChild<LLTextBox>("inventory_count")->setText(std::to_string(mVisibleItems.size()) + " предметов");
    getChild<LLButton>("all_items_tab")->setLabel(mCategory == "all" ? "● Все вещи" : "Все вещи");
    getChild<LLComboBox>("category_filter")->setValue(mCategory);
    getChild<LLButton>("favorites_tab")->setLabel(mCategory == "favorites" ? "★ Избранное" : "☆ Избранное");
    getChild<LLButton>("purchased_tab")->setLabel(mCategory == "recent" ? "◷ Недавние" : "Недавние");
    getChild<LLButton>("purchased_tab")->setToggleState(mCategory == "recent");
    getChild<LLButton>("favorites_tab")->setToggleState(mCategory == "favorites");
    getChild<LLButton>("all_items_tab")->setToggleState(mCategory == "all");
    const std::pair<const char*, const char*> rail_labels[] = {
        {"category_all", "Все"}, {"category_clothing", "Одежда"}, {"category_accessories", "Аксессуары"},
        {"category_hair", "Причёски"}, {"category_shoes", "Обувь"}, {"category_tattoos", "Тату и макияж"},
        {"category_jewelry", "Украшения"}, {"category_objects", "Декор"}, {"category_animations", "Эмоции"}, {"category_favorites", "Избранное"},
        {"category_recent", "Недавние"}
    };
    for (const auto& entry : rail_labels)
    {
        const std::string key = entry.first;
        const std::string value = key == "category_all" ? "all" : key.substr(9);
        LLButton* category_button = getChild<LLButton>(entry.first);
        const bool active = mCategory == value;
        category_button->setToggleState(active);
        category_button->setLabel(active ? std::string("✦ ") + entry.second : entry.second);
    }
    getChild<LLTextBox>("page")->setText(std::to_string(mPage + 1) + " / " + std::to_string(pages));
    getChild<LLButton>("folder_back")->setEnabled(mFolder.notNull() && mFolder != gInventory.getRootFolderID());
    getChild<LLButton>("previous")->setEnabled(mPage > 0);
    getChild<LLButton>("next")->setEnabled(mPage + 1 < pages);
    if (mSelectedItem.notNull()) selectItem(mSelectedItem);
    getChild<LLTextBox>("status")->setText(mVisibleItems.empty() ? "В этой категории пока нет предметов." : "");
}
void FSForMeInventoryBrowser::selectItem(const LLUUID& id)
{
    auto* item = gInventory.getItem(id);
    if (!item) { mSelectedItem.setNull(); mDirty = true; return; }
    mSelectedItem = id;
    const auto icon_name = LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags());
    getChild<LLThumbnailCtrl>("detail_thumbnail")->setValue(item->getThumbnailUUID().notNull() ? LLSD(item->getThumbnailUUID()) : LLSD(icon_name));
    mRecentItems.erase(std::remove(mRecentItems.begin(), mRecentItems.end(), id), mRecentItems.end());
    mRecentItems.insert(mRecentItems.begin(), id);
    if (mRecentItems.size() > 50) mRecentItems.resize(50);
    getChild<LLButton>("detail_favorite")->setLabel(get_is_favorite(id) ? "Убрать из избранного" : "В избранное");
    getChild<LLButton>("detail_favorite")->setToggleState(get_is_favorite(id));
    getChild<LLTextBox>("detail_name")->setText(item->getName());
    getChild<LLTextBox>("detail_type")->setText(item->getType() == LLAssetType::AT_CLOTHING ? "Одежда" :
        item->getType() == LLAssetType::AT_BODYPART ? "Часть тела" : item->getType() == LLAssetType::AT_OBJECT ? "Объект / аксессуар" : "Предмет инвентаря");
    auto* folder = gInventory.getCategory(item->getParentUUID());
    getChild<LLTextBox>("detail_folder")->setText(folder ? "Папка: " + folder->getName() : "");
    getChild<LLTextBox>("detail_creator")->setText(item->getCreatorUUID().isNull()
        ? "Автор: не указан" : "Автор UUID: " + item->getCreatorUUID().asString());
    getChild<LLTextBox>("detail_created")->setText(item->getCreationDate() > 0
        ? "Создан: " + LLDate(static_cast<F64>(item->getCreationDate())).asString()
        : "Дата создания: неизвестна");
    const std::string description = item->getDescription();
    getChild<LLTextBox>("detail_description")->setText(description.empty() ? "Описание не указано." : description);
    getChild<LLButton>("apply")->setEnabled(item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART || item->getType() == LLAssetType::AT_OBJECT);
    getChild<LLButton>("open_folder")->setEnabled(folder != nullptr);
    getChild<LLButton>("detail_favorite")->setEnabled(true);
    for (S32 i = 0; i < CARD_COUNT; ++i)
    {
        const S32 index = mPage * CARD_COUNT + i;
        if (index < static_cast<S32>(mVisibleItems.size()))
        {
            LLButton* select_button = getChild<LLButton>("item_select_" + std::to_string(i));
            const bool selected = mVisibleItems[index] == id;
            select_button->setLabel(selected ? "✦ Выбрано" : "Выбрать");
            select_button->setToggleState(selected);
        }
    }
}
void FSForMeInventoryBrowser::applySelected()
{
    auto* item = gInventory.getItem(mSelectedItem);
    if (!item) return;
    if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART || item->getType() == LLAssetType::AT_OBJECT)
        LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, true);
}
