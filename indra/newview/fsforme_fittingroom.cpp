/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "fsforme_fittingroom.h"
#include "llappearancemgr.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llpanel.h"
#include "lltextbox.h"
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
    getChild<LLButton>("fitting_open_inventory")->setClickedCallback([](LLUICtrl*, const LLSD&) {
        LLFloaterReg::showInstance("forme_inventory");
    });
    getChild<LLButton>("fitting_save_outfit")->setClickedCallback([this](LLUICtrl*, const LLSD&) { saveOutfit(); });
    for (S32 i = 0; i < WORN_ITEM_SLOTS; ++i)
        getChild<LLButton>("worn_remove_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            removeWornItem(i);
        });
    return true;
}
void FSForMeFittingRoom::onOpen(const LLSD&) { mDirty = true; }
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
