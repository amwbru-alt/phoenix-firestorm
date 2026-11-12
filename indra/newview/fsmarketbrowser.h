/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "llfloater.h"
#include <functional>
class LLIconCtrl;
class FSMarketBrowser : public LLFloater
{
    friend class LLFloaterReg;
public:
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;
private:
    FSMarketBrowser(const LLSD& key) : LLFloater(key) {}
    void refresh();
    void render();
    void request(LLSD data, std::function<void(FSMarketBrowser*, const LLSD&)> done);
    void add(const LLSD& item);
    void checkout(const LLSD& items);
    void submit(const LLSD& order);
    void image(LLIconCtrl* target, const std::string& url);
    void status(const std::string& message);
    std::string mMode, mItemId, mCategory, mCap;
    LLUUID mAgent, mSession;
    LLSD mItems = LLSD::emptyArray(), mProduct, mRenderedCart;
    S32 mPage = 0, mTotal = 0;
    U64 mRevision = 0;
    bool mBusy = false;
};
