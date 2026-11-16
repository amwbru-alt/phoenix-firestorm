/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "lluuid.h"
#include <string>
#include <vector>
class LLViewerInventoryItem;
class FSForMeInventoryBrowser final : public LLFloater, public LLInventoryObserver
{
    friend class LLFloaterReg;
public:
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;
    void changed(U32 mask) override;
private:
    FSForMeInventoryBrowser(const LLSD& key);
    ~FSForMeInventoryBrowser() override;
    void rebuild();
    void selectItem(const LLUUID& id);
    void applySelected();
    bool matches(const LLViewerInventoryItem* item) const;
    std::vector<LLUUID> mVisibleItems;
    LLUUID mSelectedItem;
    std::string mCategory = "all";
    std::string mQuery;
    bool mDirty = true;
};