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
    void preset();
    void populate(const LLSD& entity);
    void controls(bool enabled);
    LLUUID mTarget;
    LLSD mEntity;
    bool mPending = false;
};
#endif
