/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FS_WORLD_ENTITIES_H
#define FS_WORLD_ENTITIES_H
#include "llsd.h"
#include "lluuid.h"
#include <functional>
namespace ForMeWorldEntities
{
void update();
LLSD entity(const LLUUID& id);
std::string capability();
void setMaterial(const LLUUID& id, const LLSD& material, std::function<void(bool,const std::string&)> done);
}
#endif
