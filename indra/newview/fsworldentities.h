/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FS_WORLD_ENTITIES_H
#define FS_WORLD_ENTITIES_H
#include "llsd.h"
#include "lluuid.h"
#include <functional>
namespace ForMeWorldEntities
{
using EntityCallback = std::function<void(bool,const LLSD&)>;
void readPhysicalObject(const LLUUID& id, EntityCallback done);
void setPhysicsProfile(const LLUUID& id, const std::string& revision, const LLSD& profile, EntityCallback done);
void update();
LLSD entity(const LLUUID& id);
std::string capability();
void setMaterial(const LLUUID& id, const LLSD& material, std::function<void(bool,const std::string&)> done);
}
#endif
