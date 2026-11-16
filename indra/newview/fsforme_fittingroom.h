/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "lluuid.h"
#include <vector>
class FSForMeAvatarPreview;

class FSForMeFittingRoom final : public LLFloater, public LLInventoryObserver
{
    friend class LLFloaterReg;
public:
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    void draw() override;
    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleMouseUp(S32 x, S32 y, MASK mask) override;
    bool handleHover(S32 x, S32 y, MASK mask) override;
    bool handleScrollWheel(S32 x, S32 y, S32 clicks) override;
    void changed(U32 mask) override;
private:
    FSForMeFittingRoom(const LLSD& key);
    ~FSForMeFittingRoom() override;
    void rebuild();
    void removeWornItem(S32 index);
    void saveOutfit();
    void setCameraPreset(F32 preset);
    void adjustCameraZoom(F32 delta);
    void drawAvatarPreview();
    std::vector<LLUUID> mWornItems;
    S32 mSavedOutfitCount = 0;
    bool mDirty = true;
    FSForMeAvatarPreview* mAvatarPreview = nullptr;
    bool mDraggingPreview = false;
    S32 mLastPreviewMouseX = 0;
};
