/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "lluuid.h"
#include <vector>
class FSForMeFittingRoom final : public LLFloater, public LLInventoryObserver
{
    friend class LLFloaterReg;
public:
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;
    void changed(U32 mask) override;
private:
    FSForMeFittingRoom(const LLSD& key);
    ~FSForMeFittingRoom() override;
    void rebuild();
    void removeWornItem(S32 index);
    void saveOutfit();
    std::vector<LLUUID> mWornItems;
    S32 mSavedOutfitCount = 0;
    bool mDirty = true;
};
