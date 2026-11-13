/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#include "llviewerprecompiledheaders.h"
#include "fsworldentities.h"
#include "formeworldreplica.h"
#include "formegridpolicy.h"
#include "fscreatoraccess.h"
#include "llagent.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llstartup.h"
#include "llviewerregion.h"
#include <chrono>
#include <cmath>

namespace ForMeWorldEntities
{
namespace
{
using Clock = std::chrono::steady_clock;
ForMeWorldProtocol::Replica<LLSD> cache;
LLUUID agent, session, region;
std::string uri;
U64 generation = 0;
bool pending = false, editing = false;
std::map<std::string,std::uint64_t> minimumRevisions;
Clock::time_point next, fresh;
bool validEntity(const LLSD& value, ForMeWorldProtocol::Entity<LLSD>& entity)
{
    entity.id = value["entityId"].asString();
    if (!value.isMap() || value["schemaVersion"].asInteger() != 1 || !ForMeWorldProtocol::uuid(entity.id) || value["regionId"].asString() != region.asString()
        || !ForMeWorldProtocol::uuid(value["authorityNodeId"].asString()) || !value["revision"].isString() || !value["authorityEpoch"].isString()
        || !ForMeWorldProtocol::revision(value["revision"].asString(), entity.revision) || !ForMeWorldProtocol::revision(value["authorityEpoch"].asString(), entity.authority)
        || !value["components"].isMap() || value["components"].size() > 64) return false;
    const LLSD& transform = value["components"]["transform"];
    if (transform["schemaVersion"].asInteger() != 1 || !transform["data"].isMap()) return false;
    for (const char* axis : {"x", "y", "z"})
        if (!transform["data"]["position"][axis].isReal() && !transform["data"]["position"][axis].isInteger()) return false;
        else if (!std::isfinite(transform["data"]["position"][axis].asReal()) || std::abs(transform["data"]["position"][axis].asReal()) > 1e7) return false;
    entity.data = value; return true;
}
bool accept(const LLSD& value)
{
    if (value["protocolVersion"].asInteger() != 1 || !value["cursor"].isString() || !value["changes"].isArray()
        || value["changes"].size() > 512 || value["hasMore"].asBoolean() || !value["reset"].isBoolean()) return false;
    ForMeWorldProtocol::Page<LLSD> page;
    page.region = value["regionId"].asString(); page.epoch = value["epoch"].asString(); page.reset = value["reset"].asBoolean();
    if (!ForMeWorldProtocol::revision(value["cursor"].asString(), page.cursor)) return false;
    for (auto it = value["changes"].beginArray(); it != value["changes"].endArray(); ++it)
    {
        ForMeWorldProtocol::Change<LLSD> change;
        change.id = (*it)["entityId"].asString();
        if (!(*it)["deleted"].isBoolean() || (*it)["cursor"].asString() != value["cursor"].asString()) return false;
        change.deleted = (*it)["deleted"].asBoolean();
        if (!change.deleted && !validEntity((*it)["entity"], change.entity)) return false;
        page.changes.push_back(std::move(change));
    }
    return cache.apply(page, region.asString());
}
void poll(U64 requestGeneration, std::string cap)
{
    auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeWorldEntities", LLCore::HttpRequest::DEFAULT_POLICY_ID);
    auto request = std::make_shared<LLCore::HttpRequest>();
    LLSD body; body["action"] = "poll"; body["epoch"] = cache.epoch(); body["after"] = std::to_string(cache.cursor());
    LLSD result;
    try { result = adapter->postJsonAndSuspend(request, cap, body); } catch (...) { result = LLSD(); }
    if (generation != requestGeneration || cap != capability()) return;
    pending = false;
    auto status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
    if (status && accept(result))
    {
        fresh = Clock::now();
        for (auto it = minimumRevisions.begin(); it != minimumRevisions.end();)
        {
            auto current = cache.entities().find(it->first);
            if (current != cache.entities().end() && current->second.revision >= it->second) it = minimumRevisions.erase(it);
            else ++it;
        }
    }
    else { next = Clock::now() + std::chrono::seconds(5); if (status) cache.clear(); }
}
}
std::string capability()
{
    LLViewerRegion* current = gAgent.getRegion();
    if (gDisconnected || LLStartUp::getStartupState() != STATE_STARTED || !current || gAgent.getID().isNull()) return {};
    std::string cap = current->getCapability("ForMeWorld"); return ForMeGrid::allowedServiceURI(cap) ? cap : "";
}
void update()
{
    std::string current = capability(); LLUUID currentRegion = gAgent.getRegion() ? gAgent.getRegion()->getRegionID() : LLUUID::null;
    if (agent != gAgent.getID() || session != gAgent.getSessionID() || uri != current || region != currentRegion)
    { agent = gAgent.getID(); session = gAgent.getSessionID(); region = currentRegion; uri = current; ++generation; cache.clear(); minimumRevisions.clear(); pending = editing = false; next = {}; fresh = {}; }
    if (uri.empty() || pending || editing || Clock::now() < next) return;
    pending = true; next = Clock::now() + std::chrono::seconds(2);
    LLCoros::instance().launch("ForMeWorldEntities", [cap = uri, version = generation]() { poll(version, cap); });
}
LLSD entity(const LLUUID& id)
{
    if (uri.empty() || uri != capability() || minimumRevisions.contains(id.asString()) || Clock::now() - fresh > std::chrono::seconds(10)) return LLSD();
    const auto& entities = cache.entities(); auto it = entities.find(id.asString()); return it == entities.end() ? LLSD() : it->second.data;
}
namespace
{
void physicalRequest(const LLUUID& id, LLSD body, bool mutation, EntityCallback done)
{
    const std::string cap = capability();
    if (cap.empty() || cap != uri || editing || (mutation && !ForMeCreatorAccess::allowed())) { LLSD error; error["code"]="unavailable";done(false,error);return; }
    if (mutation) editing = true;
    body["entityId"] = id.asString();
    LLCoros::instance().launch("ForMeWorldPhysicalObject", [id,body,mutation,done,cap,version=generation]() {
        auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeWorldPhysicalObject", LLCore::HttpRequest::DEFAULT_POLICY_ID);
        auto request = std::make_shared<LLCore::HttpRequest>();LLSD result;
        try { result = adapter->postJsonAndSuspend(request,cap,body); } catch (...) { result = LLSD(); }
        if (version != generation || cap != capability()) { LLSD error;error["code"]="session";done(false,error);return; }
        if (mutation) editing = false;
        auto status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
        ForMeWorldProtocol::Entity<LLSD> validated;
        bool success = status && result["entityId"].asString()==id.asString() && validEntity(result,validated);
        if (success && mutation) { if (minimumRevisions.size()>=256) minimumRevisions.clear(); minimumRevisions[id.asString()]=validated.revision; }
        next = Clock::now()+std::chrono::seconds(1);
        done(success,result);
    });
}
}
void readPhysicalObject(const LLUUID& id, EntityCallback done)
{ LLSD request;request["action"]="physics_read";physicalRequest(id,request,false,done); }
void setPhysicsProfile(const LLUUID& id,const std::string& revision,const LLSD& profile,EntityCallback done)
{
    LLSD request;request["action"]="physics_profile";request["expectedRevision"]=revision;request["profile"]=profile;physicalRequest(id,request,true,done);
}
void setMaterial(const LLUUID& id, const LLSD& material, std::function<void(bool,const std::string&)> done)
{
    const LLSD target = entity(id); const std::string cap = capability();
    if (!ForMeCreatorAccess::allowed() || !target.isMap() || cap.empty() || editing) { done(false, "unavailable"); return; }
    editing = true;
    LLCoros::instance().launch("ForMeWorldMaterial", [id, material, done, cap, target, version = generation]() {
        auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeWorldMaterial", LLCore::HttpRequest::DEFAULT_POLICY_ID);
        auto request = std::make_shared<LLCore::HttpRequest>();
        LLSD body; body["action"] = "material"; body["entityId"] = id.asString(); body["expectedRevision"] = target["revision"]; body["material"] = material;
        LLSD result;
        try { result = adapter->postJsonAndSuspend(request, cap, body); } catch (...) { result = LLSD(); }
        if (generation != version || cap != capability()) { done(false, "session"); return; }
        editing = false; next = Clock::now() + std::chrono::seconds(1);
        auto status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
        // The next ordinary poll obtains the changed revision; a mutation response cannot advance the subscription cursor.
        std::uint64_t updatedRevision = 0;
        bool saved = status && result["entityId"].asString() == id.asString() && result["regionId"].asString() == region.asString()
            && result["revision"].isString() && ForMeWorldProtocol::revision(result["revision"].asString(), updatedRevision);
        if (saved) { if (minimumRevisions.size() >= 256) minimumRevisions.clear(); minimumRevisions[id.asString()] = updatedRevision; }
        done(saved, result["code"].asString());
    });
}
}
