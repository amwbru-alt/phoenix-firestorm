/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "fsformeinventory.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llbutton.h"
#include "lliconctrl.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llfloaterreg.h"
#include "llpanel.h"
#include "llinventorytype.h"
#include "llsearcheditor.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"
#include "llviewerinventory.h"
#include <algorithm>
#include <cctype>
#include <set>

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
FSForMeInventoryBrowser::~FSForMeInventoryBrowser() { gInventory.removeObserver(this); }

bool FSForMeInventoryBrowser::postBuild()
{
    gInventory.addObserver(this);
    getChild<LLSearchEditor>("query")->setCommitCallback([this](LLUICtrl*, const LLSD&) {
        mQuery = getChild<LLSearchEditor>("query")->getValue().asString();
        mPage = 0; mDirty = true;
    });
    getChild<LLButton>("refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { mDirty = true; });
    for (const auto& category : {std::make_pair("category_all", "all"), std::make_pair("category_clothing", "clothing"),
                                 std::make_pair("category_accessories", "accessories"), std::make_pair("category_objects", "objects"),
                                 std::make_pair("category_animations", "animations")})
        getChild<LLButton>(category.first)->setClickedCallback([this, value = std::string(category.second)](LLUICtrl*, const LLSD&) {
            mCategory = value; mPage = 0; mDirty = true;
        });
    getChild<LLButton>("previous")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if (mPage > 0) { --mPage; mDirty = true; } });
    getChild<LLButton>("next")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if ((mPage + 1) * CARD_COUNT < static_cast<S32>(mVisibleItems.size())) { ++mPage; mDirty = true; } });
    for (S32 i = 0; i < CARD_COUNT; ++i)
        getChild<LLButton>("item_select_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            const S32 index = mPage * CARD_COUNT + i;
            if (index < static_cast<S32>(mVisibleItems.size())) selectItem(mVisibleItems[index]);
        });
    getChild<LLButton>("apply")->setClickedCallback([this](LLUICtrl*, const LLSD&) { applySelected(); });
    getChild<LLButton>("open_folder")->setClickedCallback([this](LLUICtrl*, const LLSD&) {
        if (auto* item = gInventory.getItem(mSelectedItem)) LLFloaterReg::showInstance("inventory", LLSD().with("folder_id", item->getParentUUID()));
    });
    return true;
}
void FSForMeInventoryBrowser::onOpen(const LLSD&)
{
    mSelectedItem.setNull();
    mDirty = true;
}
void FSForMeInventoryBrowser::changed(U32)
{
    mDirty = true;
}
void FSForMeInventoryBrowser::draw()
{
    if (mDirty) rebuild();
    LLFloater::draw();
}
bool FSForMeInventoryBrowser::matches(const LLViewerInventoryItem* item) const
{
    if (!item) return false;
    const auto type = item->getType();
    if (mCategory == "clothing" && type != LLAssetType::AT_CLOTHING && type != LLAssetType::AT_BODYPART) return false;
    if (mCategory == "accessories" && (type != LLAssetType::AT_OBJECT || item->getInventoryType() != LLInventoryType::IT_WEARABLE)) return false;
    if (mCategory == "objects" && (type != LLAssetType::AT_OBJECT || item->getInventoryType() == LLInventoryType::IT_WEARABLE)) return false;
    if (mCategory == "animations" && type != LLAssetType::AT_ANIMATION && type != LLAssetType::AT_GESTURE) return false;
    return mQuery.empty() || lower(item->getName()).find(lower(mQuery)) != std::string::npos;
}
void FSForMeInventoryBrowser::rebuild()
{
    mDirty = false;
    std::vector<LLUUID> all;
    std::set<LLUUID> visited;
    collectItems(gInventory.getRootFolderID(), all, visited);
    std::sort(all.begin(), all.end(), [](const LLUUID& a, const LLUUID& b) {
        const auto* left = gInventory.getItem(a); const auto* right = gInventory.getItem(b);
        return left && right ? lower(left->getName()) < lower(right->getName()) : a.asString() < b.asString();
    });
    mVisibleItems.clear();
    for (const auto& id : all) if (matches(gInventory.getItem(id))) mVisibleItems.push_back(id);
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
        getChild<LLIconCtrl>("item_icon_" + suffix)->setValue(LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags()));
        getChild<LLTextBox>("item_name_" + suffix)->setText(item->getName());
        getChild<LLButton>("item_select_" + suffix)->setLabel(mSelectedItem == item->getUUID() ? "Выбрано" : "Выбрать");
    }
    grid->reshape(644, 560);
    getChild<LLTextBox>("inventory_count")->setText(std::to_string(mVisibleItems.size()) + " предметов");
    getChild<LLTextBox>("page")->setText(std::to_string(mPage + 1) + " / " + std::to_string(pages));
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
    getChild<LLIconCtrl>("detail_icon")->setValue(LLInventoryIcon::getIconName(item->getType(), item->getInventoryType(), item->getFlags()));
    getChild<LLTextBox>("detail_name")->setText(item->getName());
    getChild<LLTextBox>("detail_type")->setText(item->getType() == LLAssetType::AT_CLOTHING ? "Одежда" :
        item->getType() == LLAssetType::AT_BODYPART ? "Часть тела" : item->getType() == LLAssetType::AT_OBJECT ? "Объект / аксессуар" : "Предмет инвентаря");
    auto* folder = gInventory.getCategory(item->getParentUUID());
    getChild<LLTextBox>("detail_folder")->setText(folder ? "Папка: " + folder->getName() : "");
    getChild<LLButton>("apply")->setEnabled(item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART || item->getType() == LLAssetType::AT_OBJECT);
    getChild<LLButton>("open_folder")->setEnabled(folder != nullptr);
    for (S32 i = 0; i < CARD_COUNT; ++i)
        const S32 index = mPage * CARD_COUNT + i;
        if (index < static_cast<S32>(mVisibleItems.size()))
            getChild<LLButton>("item_select_" + std::to_string(i))->setLabel(mVisibleItems[index] == id ? "Выбрано" : "Выбрать");
}
void FSForMeInventoryBrowser::applySelected()
{
    auto* item = gInventory.getItem(mSelectedItem);
    if (!item) return;
    if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART || item->getType() == LLAssetType::AT_OBJECT)
        LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, true);
}
