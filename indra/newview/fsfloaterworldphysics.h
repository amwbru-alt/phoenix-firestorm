/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FS_FLOATER_WORLD_PHYSICS_H
#define FS_FLOATER_WORLD_PHYSICS_H
#include "llfloater.h"
class FSFloaterWorldPhysics final : public LLFloater
{
public:
    explicit FSFloaterWorldPhysics(const LLSD& key) : LLFloater(key) {}
    bool postBuild() override;
    void onOpen(const LLSD& key) override;
private:
    void refresh();
    void apply();
    LLUUID mTarget;
    bool mPending = false;
};
#endif
