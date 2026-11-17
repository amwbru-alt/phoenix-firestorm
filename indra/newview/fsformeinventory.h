/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "lluuid.h"
#include <string>
class FSForMeInventoryAvatarPreview;
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
    void onClose(bool app_quitting) override;
    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleMouseUp(S32 x, S32 y, MASK mask) override;
    bool handleHover(S32 x, S32 y, MASK mask) override;
    bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;
    void drawAvatarPreview();
    bool matches(const LLViewerInventoryItem* item) const;
    std::vector<LLUUID> mVisibleItems;
    std::vector<LLUUID> mRecentItems;
    LLUUID mSelectedItem;
    LLUUID mFolder;
    std::string mCategory = "all";
    std::string mQuery;
    S32 mPage = 0;
    bool mDirty = true;
    FSForMeInventoryAvatarPreview* mAvatarPreview = nullptr;
    bool mDraggingPreview = false;
    S32 mLastPreviewMouseX = 0;
};