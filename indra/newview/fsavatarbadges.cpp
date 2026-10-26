/** Server-authoritative self badge. No avatar-object echo or local color preference is required. */
#include "llviewerprecompiledheaders.h"
#include "fsavatarbadges.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llstartup.h"
#include "llvoavatar.h"
#include <chrono>

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
