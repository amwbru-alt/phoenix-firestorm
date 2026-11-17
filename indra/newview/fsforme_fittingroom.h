/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include "llinventoryobserver.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include <string>
#include <vector>
class FSForMeAvatarPreview;
class LLViewerInventoryItem;

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
    bool handleKeyHere(KEY key, MASK mask) override;
    void changed(U32 mask) override;
private:
    FSForMeFittingRoom(const LLSD& key);
    ~FSForMeFittingRoom() override;
    void rebuild();
    void rebuildLookbook();
    void selectLookbookPhoto(const std::string& id);
    void navigateLookbookPhoto(S32 delta);
    void setLookbookFullscreenMode(bool enabled);
    void drawLookbookThumbnails();
    void drawLookbookPhotoPreview();
    void drawLookbookComparison();
    void saveSelectedLookbookMetadata();
    void assignLookbookCompareSlot(bool first);
    void setCategory(const std::string& category);
    bool matchesCategory(const LLViewerInventoryItem* item) const;
    void removeWornItem(S32 index);
    void saveOutfit();
    void compareOutfits();
    void updateSelectedOutfitDetails();
    void setCameraPreset(F32 preset);
    void adjustCameraZoom(F32 delta);
    void drawAvatarPreview();
    std::vector<LLUUID> mWornItems;
    S32 mSavedOutfitCount = 0;
    S32 mLookbookPage = 0;
    bool mLookbookPreviewMode = false;
    bool mLookbookCompareMode = false;
    std::string mLookbookCompareAId;
    std::string mLookbookCompareBId;
    LLPointer<LLViewerFetchedTexture> mLookbookCompareATexture;
    LLPointer<LLViewerFetchedTexture> mLookbookCompareBTexture;
    std::string mSelectedLookbookId;
    std::vector<std::string> mLookbookIds;
    LLSD mFilteredLookbookEntries;
    std::string mLookbookFilterCategory = "all";
    bool mLookbookFavoritesOnly = false;
    bool mLookbookOldestFirst = false;
    bool mLookbookFullscreenMode = false;
    S32 mLookbookPreviewWidth = 512;
    S32 mLookbookPreviewHeight = 307;
    std::vector<LLPointer<LLViewerFetchedTexture>> mLookbookTextures;
    LLPointer<LLViewerFetchedTexture> mLookbookPreviewTexture;
    LLUUID mDetailsOutfit;
    S32 mWornPage = 0;
    bool mDirty = true;
    std::string mCategory = "all";
    FSForMeAvatarPreview* mAvatarPreview = nullptr;
    bool mDraggingPreview = false;
    S32 mLastPreviewMouseX = 0;
    S32 mLastPreviewMouseY = 0;
};
