/** Server-authoritative self badge. No avatar-object echo or local color preference is required. */
#include "llviewerprecompiledheaders.h"
#include "fsavatarbadges.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llstartup.h"
#include "llvoavatar.h"
#include <chrono>
#include "llhttpnode.h"
#include "llstatusbar.h"
#include "llavatarpropertiesprocessor.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llcallingcard.h"
#include "llaisapi.h"
#include "llmarketplacefunctions.h"

namespace ForMeAvatarBadges
{
namespace
{
ForMeBadge::Data s_badge;
LLUUID s_agent;
std::string s_cap;
U64 s_generation = 0;
bool s_pending = false;
using Clock = std::chrono::steady_clock;
Clock::time_point s_next, s_fetched;
std::string capability()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (LLStartUp::getStartupState() != STATE_STARTED || !region || gAgent.getID().isNull()) return "";
    const std::string cap = region->getCapability("ForMeAvatarBadge");
    return ForMeGrid::allowedServiceURI(cap) ? cap : "";
}
void fetch(LLUUID agent, std::string cap, U64 generation)
{
    auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeSelfBadge", LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCore::HttpRequest::ptr_t request = std::make_shared<LLCore::HttpRequest>();
    LLSD result;
    try { result = adapter->getJsonAndSuspend(request, cap); }
    catch (...) { result = LLSD(); }
    if (generation != s_generation || agent != gAgent.getID() || cap != capability()) return;
    s_pending = false;
    const LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(
        result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
    ForMeBadge::Data next;
    if (status && result["version"].asInteger() == 1 && result["agent_id"].asUUID() == agent)
        next = ForMeBadge::parse(result["id"].asString(), result["label"].asString(), result["badge_color"].asString(),
            result["name_color"].asString(), result["chat_color"].asString());
    const bool changed = s_badge.stamp != next.stamp;
    s_badge = next;
    s_fetched = Clock::now();
    if (changed) LLVOAvatar::invalidateNameTag(agent);
}
}
ForMeBadge::Data self()
{
    if (s_agent != gAgent.getID() || s_cap.empty() || s_cap != capability()
        || Clock::now() - s_fetched > std::chrono::seconds(25)) return {};
    return s_badge;
}
void updateSelf()
{
    const std::string cap = capability();
    if (s_agent != gAgent.getID() || s_cap != cap)
    {
        const LLUUID old_agent = s_agent;
        s_agent = gAgent.getID(); s_cap = cap; ++s_generation;
        s_pending = false; s_badge = {}; s_next = {};
        if (old_agent.notNull()) LLVOAvatar::invalidateNameTag(old_agent);
    }
    const auto now = Clock::now();
    if (s_cap.empty() || s_pending || now < s_next) return;
    s_next = now + std::chrono::seconds(10); s_pending = true;
    LLCoros::instance().launch("ForMeSelfBadge", [agent = s_agent, uri = s_cap, generation = s_generation]() { fetch(agent, uri, generation); });
}
}


namespace {
class LLForMeState final : public LLHTTPNode
{
    void post(ResponsePtr response, const LLSD& context, const LLSD& input) const override
    {
        const LLSD& body = input["body"];
        LLViewerRegion* region = gAgent.getRegion();
        if (!region || LLStartUp::getStartupState() != STATE_STARTED
            || !ForMeGrid::allowedServiceURI(region->getCapability("ForMeState"))
            || body["user_id"].asUUID() != gAgent.getID() || body["session_id"].asUUID() != gAgent.getSessionID()
            || body["region_id"].asUUID() != region->getRegionID() || !body["versions"].isMap()) return;
        static LLUUID agent, session, previous_region;
        static LLSD previous;
        if (agent != gAgent.getID() || session != gAgent.getSessionID() || previous_region != region->getRegionID())
        { agent = gAgent.getID(); session = gAgent.getSessionID(); previous_region = region->getRegionID(); previous = LLSD(); }
        const LLSD versions = body["versions"];
        const auto changed = [&](const char* key) { return versions[key].isString() && versions[key].asString().size() == 64 && versions[key].asString() != previous[key].asString(); };
        if (changed("balance")) LLStatusBar::sendMoneyBalanceRequest();
        if (changed("profile") || changed("premium")) LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(gAgent.getID());
        if (changed("friends") && body["friendships_complete"].asBoolean()) LLAvatarTracker::instance().syncForMeRelationships(body["friendships"]);
        if (changed("inventory"))
        {
            if (body["inventory_folders"].isArray() && body["inventory_folders"].size() <= 10000)
                for (auto it = body["inventory_folders"].beginArray(); it != body["inventory_folders"].endArray(); ++it)
                {
                    const LLSD& row = *it;
                    LLUUID id = row["id"].asUUID(); auto* category = gInventory.getCategory(id);
                    if (category && category->getOwnerID() == gAgent.getID() && category->getVersion() != row["version"].asInteger())
                    { category->setVersion(LLViewerInventoryCategory::VERSION_UNKNOWN); gInventory.fetchDescendentsOf(id); }
                }
            gInventory.fetchDescendentsOf(gInventory.getRootFolderID());
        }
        if (changed("listings") && previous.isMap()) LLMarketplaceData::instance().getSLMListings();
        const LLSD& events = body["changes"]["events"];
        if (events.isArray() && events.size() <= 100)
            for (auto it = events.beginArray(); it != events.endArray(); ++it)
            {
                const LLUUID id = (*it)["entity_id"].asUUID();
                if (id.isNull()) continue;
                const std::string topic = (*it)["topic"].asString();
                if (topic == "profile" || topic == "premium")
                {
                    auto* profiles = LLAvatarPropertiesProcessor::getInstance();
                    if (id == gAgent.getID() || profiles->hasObservers(id)) profiles->sendAvatarPropertiesRequest(id);
                }
                else if ((topic == "thumbnail" || topic == "favorite") && AISAPI::isAvailable())
                {
                    if (auto* category = gInventory.getCategory(id); category && category->getOwnerID() == gAgent.getID())
                        AISAPI::FetchCategoryChildren(id);
                    else if (auto* item = gInventory.getItem(id); item && item->getPermissions().getOwner() == gAgent.getID())
                        AISAPI::FetchItem(id, AISAPI::INVENTORY);
                }
            }
        previous = versions;
    }
};
LLHTTPRegistration<LLForMeState> gForMeStateRegistration("/message/ForMeState");
}
