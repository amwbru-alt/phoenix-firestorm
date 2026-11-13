/**
 * @file llspeakers.cpp
 * @brief Management interface for muting and controlling volume of residents currently speaking
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llspeakers.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llcallingcard.h"
#include "llappviewer.h"
#include "llimview.h"
#include "llgroupmgr.h"
#include "llsdutil.h"
#include "lluicolortable.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "llcorehttputil.h"

#include "lfsimfeaturehandler.h"    // <FS:CR> Opensim

extern LLControlGroup gSavedSettings;

const LLColor4 INACTIVE_COLOR(0.3f, 0.3f, 0.3f, 0.5f);
const LLColor4 ACTIVE_COLOR(0.5f, 0.5f, 0.5f, 1.f);

LLSpeaker::LLSpeaker(const LLUUID& id, const std::string& name, const ESpeakerType type) :
    mStatus(LLSpeaker::STATUS_TEXT_ONLY),
    mLastSpokeTime(0.f),
    mHasSpoken(false),

    mDotColor(LLColor4::white),
    mID(id),
    mTyping(false),
    mSortIndex(0),
    mType(type),
    mIsModerator(false),

    mModeratorMutedText(false)
{
    if (name.empty() && type == SPEAKER_AGENT)
    {
        lookupName();
    }
    else
    {
        mDisplayName = name;
    }
}


void LLSpeaker::lookupName()
{
    if (mDisplayName.empty())
    {
        // <FS:Zi> Crash fix on login
        if(!gCacheName)
            return;
        // </FS:Zi>
        mAvatarNameCacheConnection = LLAvatarNameCache::get(mID, boost::bind(&LLSpeaker::onNameCache, this, _1, _2)); // todo: can be group???
    }
}

void LLSpeaker::onNameCache(const LLUUID& id, const LLAvatarName& av_name)
{
    mDisplayName = av_name.getUserName();
}


LLSpeakerUpdateSpeakerEvent::LLSpeakerUpdateSpeakerEvent(LLSpeaker* source)
: LLEvent(source, "Speaker update speaker event"),
  mSpeakerID (source->mID)
{
}

LLSD LLSpeakerUpdateSpeakerEvent::getValue()
{
    LLSD ret;
    ret["id"] = mSpeakerID;
    return ret;
}

LLSpeakerUpdateModeratorEvent::LLSpeakerUpdateModeratorEvent(LLSpeaker* source)
: LLEvent(source, "Speaker add moderator event"),
  mSpeakerID (source->mID),
  mIsModerator (source->mIsModerator)
{
}

LLSD LLSpeakerUpdateModeratorEvent::getValue()
{
    LLSD ret;
    ret["id"] = mSpeakerID;
    ret["is_moderator"] = mIsModerator;
    return ret;
}

LLSpeakerTextModerationEvent::LLSpeakerTextModerationEvent(LLSpeaker* source)
: LLEvent(source, "Speaker text moderation event")
{
}

LLSD LLSpeakerTextModerationEvent::getValue()
{
    return std::string("text");
}


LLSpeakerListChangeEvent::LLSpeakerListChangeEvent(LLSpeakerMgr* source, const LLUUID& speaker_id)
: LLEvent(source, "Speaker added/removed from speaker mgr"),
  mSpeakerID(speaker_id)
{
}

LLSD LLSpeakerListChangeEvent::getValue()
{
    return mSpeakerID;
}

// helper sort class
struct LLSortRecentSpeakers
{
    bool operator()(const LLPointer<LLSpeaker> lhs, const LLPointer<LLSpeaker> rhs) const;
};

bool LLSortRecentSpeakers::operator()(const LLPointer<LLSpeaker> lhs, const LLPointer<LLSpeaker> rhs) const
{
    // Sort first on status
    if (lhs->mStatus != rhs->mStatus)
    {
        return (lhs->mStatus < rhs->mStatus);
    }

    // and then on last speaking time
    if(lhs->mLastSpokeTime != rhs->mLastSpokeTime)
    {
        return (lhs->mLastSpokeTime > rhs->mLastSpokeTime);
    }

    // and finally (only if those are both equal), on name.
    return( lhs->mDisplayName.compare(rhs->mDisplayName) < 0 );
}

LLSpeakerActionTimer::LLSpeakerActionTimer(action_callback_t action_cb, F32 action_period, const LLUUID& speaker_id)
: LLEventTimer(action_period)
, mActionCallback(action_cb)
, mSpeakerId(speaker_id)
{
}

bool LLSpeakerActionTimer::tick()
{
    if (mActionCallback)
    {
        return (bool)mActionCallback(mSpeakerId);
    }
    return true;
}

void LLSpeakerActionTimer::unset()
{
    mActionCallback = 0;
}

LLSpeakersDelayActionsStorage::LLSpeakersDelayActionsStorage(LLSpeakerActionTimer::action_callback_t action_cb, F32 action_delay)
: mActionCallback(action_cb)
, mActionDelay(action_delay)
{
}

LLSpeakersDelayActionsStorage::~LLSpeakersDelayActionsStorage()
{
    removeAllTimers();
}

void LLSpeakersDelayActionsStorage::setActionTimer(const LLUUID& speaker_id)
{
    bool not_found = true;
    if (mActionTimersMap.size() > 0)
    {
        not_found = mActionTimersMap.find(speaker_id) == mActionTimersMap.end();
    }

    // If there is already a started timer for the passed UUID don't do anything.
    if (not_found)
    {
        // Starting a timer to remove an participant after delay is completed
        mActionTimersMap.insert(LLSpeakerActionTimer::action_value_t(speaker_id,
            new LLSpeakerActionTimer(
                boost::bind(&LLSpeakersDelayActionsStorage::onTimerActionCallback, this, _1),
                mActionDelay, speaker_id)));
    }
}

void LLSpeakersDelayActionsStorage::unsetActionTimer(const LLUUID& speaker_id)
{
    if (mActionTimersMap.size() == 0) return;

    LLSpeakerActionTimer::action_timer_iter_t it_speaker = mActionTimersMap.find(speaker_id);

    if (it_speaker != mActionTimersMap.end())
    {
        it_speaker->second->unset();
        mActionTimersMap.erase(it_speaker);
    }
}

void LLSpeakersDelayActionsStorage::removeAllTimers()
{
    LLSpeakerActionTimer::action_timer_iter_t iter = mActionTimersMap.begin();
    for (; iter != mActionTimersMap.end(); ++iter)
    {
        delete iter->second;
    }
    mActionTimersMap.clear();
}

bool LLSpeakersDelayActionsStorage::onTimerActionCallback(const LLUUID& speaker_id)
{
    unsetActionTimer(speaker_id);

    if (mActionCallback)
    {
        mActionCallback(speaker_id);
    }

    return true;
}

bool LLSpeakersDelayActionsStorage::isTimerStarted(const LLUUID& speaker_id)
{
    return (mActionTimersMap.size() > 0) && (mActionTimersMap.find(speaker_id) != mActionTimersMap.end());
}

//
// LLSpeakerMgr
//

LLSpeakerMgr::LLSpeakerMgr(const LLUUID& session_id) :
    mSessionID(session_id),


    mSpeakerListUpdated(false)
{
    mGetListTime.reset();
    static LLUICachedControl<F32> remove_delay ("SpeakerParticipantRemoveDelay", 10.0);

    mSpeakerDelayRemover = new LLSpeakersDelayActionsStorage(boost::bind(&LLSpeakerMgr::removeSpeaker, this, _1), remove_delay);
}

LLSpeakerMgr::~LLSpeakerMgr()
{
    delete mSpeakerDelayRemover;
}

LLPointer<LLSpeaker> LLSpeakerMgr::setSpeaker(const LLUUID& id, const std::string& name, LLSpeaker::ESpeakerStatus status, LLSpeaker::ESpeakerType type)
{
    LLUUID session_id = getSessionID();
    if (id.isNull() || (id == session_id))
    {
        return NULL;
    }

    LLPointer<LLSpeaker> speakerp;
    if (mSpeakers.find(id) == mSpeakers.end())
    {
        speakerp = new LLSpeaker(id, name, type);
        speakerp->mStatus = status;
        mSpeakers.insert(std::make_pair(speakerp->mID, speakerp));
        mSpeakersSorted.push_back(speakerp);
        LL_DEBUGS("Speakers") << "Added speaker " << id << LL_ENDL;
        fireEvent(new LLSpeakerListChangeEvent(this, speakerp->mID), "add");
    }
    else
    {
        speakerp = findSpeaker(id);
        if (speakerp.notNull())
        {
            // keep highest priority status (lowest value) instead of overriding current value
            speakerp->mStatus = llmin(speakerp->mStatus, status);
            // RN: due to a weird behavior where IMs from attached objects come from the wearer's agent_id
            // we need to override speakers that we think are objects when we find out they are really
            // residents
            if (type == LLSpeaker::SPEAKER_AGENT)
            {
                speakerp->mType = LLSpeaker::SPEAKER_AGENT;
                speakerp->lookupName();
            }
        }
        else
        {
            LL_WARNS("Speakers") << "Speaker " << id << " not found" << LL_ENDL;
        }
    }

    mSpeakerDelayRemover->unsetActionTimer(speakerp->mID);
    return speakerp;
}




void LLSpeakerMgr::update(bool resort_ok)
{
    if (resort_ok)
    {
        updateSpeakerList();
        std::sort(mSpeakersSorted.begin(), mSpeakersSorted.end(), LLSortRecentSpeakers());
    }
    S32 index = 0;
    for (auto& participant : mSpeakersSorted) participant->mSortIndex = index++;
}

void LLSpeakerMgr::updateSpeakerList()
    {
        // If not, check if the list is empty, except if it's Nearby Chat (session_id NULL).
        LLUUID session_id = getSessionID();
        if (!session_id.isNull() && !mSpeakerListUpdated)
        {
            // If the list is empty, we update it with whatever we have locally so that it doesn't stay empty too long.
            // *TODO: Fix the server side code that sometimes forgets to send back the list of participants after a chat started.
            // (IOW, fix why we get no ChatterBoxSessionAgentListUpdates message after the initial ChatterBoxSessionStartReply)
            LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
            if (!session) return;
            if (session->isGroupSessionType() && (mSpeakers.size() <= 1))
            {
                // For groups, we need to hit the group manager.
                // Note: The session uuid and the group uuid are actually one and the same. If that was to change, this will fail.
                LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(session_id);

                if (gdatap && gdatap->isMemberDataComplete() && !gdatap->mMembers.empty())
                {
                    // Add group members when we get the complete list (note: can take a while before we get that list)
                    LLGroupMgrGroupData::member_list_t::iterator member_it = gdatap->mMembers.begin();
                    const S32 load_group_max_members = gSavedSettings.getS32("ChatLoadGroupMaxMembers");
                    S32 updated = 0;
                    while (member_it != gdatap->mMembers.end())
                    {
                        LLGroupMemberData* member = member_it->second;
                        LLUUID id = member_it->first;
                        // Add only members who are online and not already in the list
                        if ((member->getOnlineStatus() == "Online") && (mSpeakers.find(id) == mSpeakers.end()))
                        {

                            LLPointer<LLSpeaker> speakerp = setSpeaker(id, "", LLSpeaker::STATUS_TEXT_ONLY, LLSpeaker::SPEAKER_AGENT);
                            speakerp->mIsModerator = ((member->getAgentPowers() & GP_SESSION_MODERATOR) == GP_SESSION_MODERATOR);
                            updated++;
                        }
                        ++member_it;
                        // Limit the number of "manually updated" participants to a reasonable number to avoid severe fps drop
                        // *TODO : solve the perf issue of having several hundreds of widgets in the conversation list
                        if (updated >= load_group_max_members)
                            break;
                    }
                    mSpeakerListUpdated = true;
                }
            }
            else if (mSpeakers.size() == 0)
            {
                // For all other session type (ad-hoc, P2P), we use the initial participants targets list
                for (uuid_vec_t::iterator it = session->mInitialTargetIDs.begin();it!=session->mInitialTargetIDs.end();++it)
                {
                    // Add buddies if they are on line, add any other avatar.
                    if (!LLAvatarTracker::instance().isBuddy(*it) || LLAvatarTracker::instance().isBuddyOnline(*it))
                    {
                        setSpeaker(*it, "", LLSpeaker::STATUS_TEXT_ONLY, LLSpeaker::SPEAKER_AGENT);
                    }
                }
                mSpeakerListUpdated = true;
            }
            else
            {
                // The list has been updated the normal way (i.e. by a ChatterBoxSessionAgentListUpdates received from the server)
                mSpeakerListUpdated = true;
            }
        }
    // Always add the current agent (it has to be there...). Will do nothing if already there.

    setSpeaker(gAgentID, "", LLSpeaker::STATUS_TEXT_ONLY, LLSpeaker::SPEAKER_AGENT);
}

void LLSpeakerMgr::setSpeakerNotInChannel(LLPointer<LLSpeaker> speakerp)
{
    if  (speakerp.notNull())
    {
        speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
        speakerp->mDotColor = INACTIVE_COLOR;
        mSpeakerDelayRemover->setActionTimer(speakerp->mID);
    }
}

bool LLSpeakerMgr::removeSpeaker(const LLUUID& speaker_id)
{
    mSpeakers.erase(speaker_id);

    speaker_list_t::iterator sorted_speaker_it = mSpeakersSorted.begin();

    for(; sorted_speaker_it != mSpeakersSorted.end(); ++sorted_speaker_it)
    {
        if (speaker_id == (*sorted_speaker_it)->mID)
        {
            mSpeakersSorted.erase(sorted_speaker_it);
            break;
        }
    }

    LL_DEBUGS("Speakers") << "Removed speaker " << speaker_id << LL_ENDL;
    fireEvent(new LLSpeakerListChangeEvent(this, speaker_id), "remove");

    update(true);

    return false;
}

LLPointer<LLSpeaker> LLSpeakerMgr::findSpeaker(const LLUUID& speaker_id)
{
    //In some conditions map causes crash if it is empty(Windows only), adding check (EK)
    if (mSpeakers.size() == 0)
        return NULL;
    speaker_map_t::iterator found_it = mSpeakers.find(speaker_id);
    if (found_it == mSpeakers.end())
    {
        return NULL;
    }
    return found_it->second;
}

void LLSpeakerMgr::getSpeakerList(speaker_list_t* speaker_list, bool include_text)
{
    speaker_list->clear();
    for (speaker_map_t::iterator speaker_it = mSpeakers.begin(); speaker_it != mSpeakers.end(); ++speaker_it)
    {
        LLPointer<LLSpeaker> speakerp = speaker_it->second;
        // what about text only muted or inactive?
        if (include_text || speakerp->mStatus != LLSpeaker::STATUS_TEXT_ONLY)
        {
            speaker_list->push_back(speakerp);
        }
    }
}

const LLUUID LLSpeakerMgr::getSessionID()
{ return mSessionID; }

bool LLSpeakerMgr::isSpeakerToBeRemoved(const LLUUID& speaker_id)
{
    return mSpeakerDelayRemover && mSpeakerDelayRemover->isTimerStarted(speaker_id);
}

void LLSpeakerMgr::setSpeakerTyping(const LLUUID& speaker_id, bool typing)
{
    LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
    if (speakerp.notNull())
    {
        speakerp->mTyping = typing;
    }
}


void LLSpeakerMgr::speakerChatted(const LLUUID& speaker_id)
{
    LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
    if (speakerp.notNull())
    {
        speakerp->mLastSpokeTime = mSpeechTimer.getElapsedTimeF32();
        speakerp->mHasSpoken = true;
        fireEvent(new LLSpeakerUpdateSpeakerEvent(speakerp), "update_speaker");
    }
}


//
// LLIMSpeakerMgr
//
LLIMSpeakerMgr::LLIMSpeakerMgr(const LLUUID& session_id) : LLSpeakerMgr(session_id)
{
}

void LLIMSpeakerMgr::updateSpeakerList()
{

    // rely on user list reported by sim

    LLSpeakerMgr::updateSpeakerList();

    return;
}

void LLIMSpeakerMgr::setSpeakers(const LLSD& speakers)
{
    if ( !speakers.isMap() ) return;

    if ( speakers.has("agent_info") && speakers["agent_info"].isMap() )
    {
        LLSD::map_const_iterator speaker_it;
        for(speaker_it = speakers["agent_info"].beginMap();
            speaker_it != speakers["agent_info"].endMap();
            ++speaker_it)
        {
            LLUUID agent_id(speaker_it->first);

            LLPointer<LLSpeaker> speakerp = setSpeaker(
                agent_id,
                LLStringUtil::null,
                LLSpeaker::STATUS_TEXT_ONLY);

            if ( speaker_it->second.isMap() )
            {
                if (speakerp.isNull()) continue;
                const LLSD& info = speaker_it->second.has("info") ? speaker_it->second["info"] : speaker_it->second;
                bool is_moderator = speakerp->mIsModerator;
                speakerp->mIsModerator = info["is_moderator"];
                speakerp->mModeratorMutedText = info["mutes"]["text"];
                // Fire event only if moderator changed
                if ( is_moderator != speakerp->mIsModerator )
                {
                    LL_DEBUGS("Speakers") << "Speaker " << agent_id << (is_moderator ? "is now" : "no longer is") << " a moderator" << LL_ENDL;
                    fireEvent(new LLSpeakerUpdateModeratorEvent(speakerp), "update_moderator");
                }
            }
        }
    }
    else if ( speakers.has("agents" ) && speakers["agents"].isArray() )
    {
        //older, more decprecated way.  Need here for
        //using older version of servers
        LLSD::array_const_iterator speaker_it;
        for(speaker_it = speakers["agents"].beginArray();
            speaker_it != speakers["agents"].endArray();
            ++speaker_it)
        {
            const LLUUID agent_id = (*speaker_it).asUUID();

            LLPointer<LLSpeaker> speakerp = setSpeaker(
                agent_id,
                LLStringUtil::null,
                LLSpeaker::STATUS_TEXT_ONLY);
        }
    }
}

void LLIMSpeakerMgr::updateSpeakers(const LLSD& update)
{
    if ( !update.isMap() ) return;

    if ( update.has("agent_updates") && update["agent_updates"].isMap() )
    {
        LLSD::map_const_iterator update_it;
        for(
            update_it = update["agent_updates"].beginMap();
            update_it != update["agent_updates"].endMap();
            ++update_it)
        {
            LLUUID agent_id(update_it->first);
            LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);

            LLSD agent_data = update_it->second;

            if (agent_data.isMap() && agent_data.has("transition"))
            {
                if (agent_data["transition"].asString() == "LEAVE")
                {
                    setSpeakerNotInChannel(speakerp);
                }
                else if (agent_data["transition"].asString() == "ENTER")
                {
                    // add or update speaker
                    speakerp = setSpeaker(agent_id);

                    // <FS:Zi> add new speakers to participants list
                    if (speakerp.notNull() && agent_data.has("has_spoken"))
                    {
                        speakerp->mLastSpokeTime = mSpeechTimer.getElapsedTimeF32();
                        speakerp->mHasSpoken = true;
                    }
                    // </FS:Zi>
                }
                else
                {
                    LL_WARNS() << "bad membership list update from 'agent_updates' for agent " << agent_id << ", transition " << ll_print_sd(agent_data["transition"]) << LL_ENDL;
                }
            }

            if (speakerp.isNull()) continue;

            // should have a valid speaker from this point on
            if (agent_data.isMap() && agent_data.has("info"))
            {
                LLSD agent_info = agent_data["info"];

                if (agent_info.has("is_moderator"))
                {
                    bool is_moderator = speakerp->mIsModerator;
                    speakerp->mIsModerator = agent_info["is_moderator"];
                    // Fire event only if moderator changed
                    if ( is_moderator != speakerp->mIsModerator )
                    {
                        LL_DEBUGS("Speakers") << "Speaker " << agent_id << (is_moderator ? "is now" : "no longer is") << " a moderator" << LL_ENDL;
                        fireEvent(new LLSpeakerUpdateModeratorEvent(speakerp), "update_moderator");
                    }
                }

                if (agent_info.has("mutes"))
                {
                    speakerp->mModeratorMutedText = agent_info["mutes"]["text"];
                }
            }
        }
    }
    else if ( update.has("updates") && update["updates"].isMap() )
    {
        LLSD::map_const_iterator update_it;
        for (
            update_it = update["updates"].beginMap();
            update_it != update["updates"].endMap();
            ++update_it)
        {
            LLUUID agent_id(update_it->first);
            LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);

            std::string agent_transition = update_it->second.asString();
            if (agent_transition == "LEAVE")
            {
                setSpeakerNotInChannel(speakerp);
            }
            else if ( agent_transition == "ENTER")
            {
                // add or update speaker
                speakerp = setSpeaker(agent_id);
            }
            else
            {
                LL_WARNS() << "bad membership list update from 'updates' for agent " << agent_id << ", transition " << agent_transition << LL_ENDL;
            }
        }
    }
}

// <FS:Zi> make text chat block in groups not a toggle to prevent accidental unmuting
// void LLIMSpeakerMgr::toggleAllowTextChat(const LLUUID& speaker_id)
void LLIMSpeakerMgr::allowTextChat(const LLUUID& speaker_id, bool allow)
{
    // <FS:Zi> Don't test for presence in the group participants list, as the list
    //         is unreliable and this leads to situations where people can't be
    //         chat muted at all anymore
    // LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);

    if (getSessionID().isNull()) return;

    std::string url = gAgent.getRegionCapability("ChatSessionRequest");
    LLSD data;
    data["method"] = "mute update";
    data["session-id"] = getSessionID();
    data["params"] = LLSD::emptyMap();
    data["params"]["agent_id"] = speaker_id;
    data["params"]["mute_info"] = LLSD::emptyMap();
    // <FS:Zi> make text chat block in groups not a toggle to prevent accidental unmuting
    //current value represents ability to type, so invert
    // data["params"]["mute_info"]["text"] = !speakerp->mModeratorMutedText;
    data["params"]["mute_info"]["text"] = !allow;

    LLCoros::instance().launch("LLIMSpeakerMgr::moderationActionCoro",
        boost::bind(&LLIMSpeakerMgr::moderationActionCoro, this, url, data));
}


void LLIMSpeakerMgr::moderationActionCoro(std::string url, LLSD action)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("moderationActionCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();

    httpOpts->setWantHeaders(true);

    LLUUID sessionId = action["session-id"];

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, action, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        if (gIMMgr)
        {
            //403 == you're not a mod
            //should be disabled if you're not a moderator
            if (status == LLCore::HttpStatus(HTTP_FORBIDDEN))
            {
                gIMMgr->showSessionEventError(
                    "mute",
                    "not_a_mod_error",
                    sessionId);
            }
            else
            {
                gIMMgr->showSessionEventError(
                    "mute",
                    "generic_request_error",
                    sessionId);
            }
        }
        return;
    }
}


//
// LLActiveSpeakerMgr
//






//
// LLLocalSpeakerMgr
//

LLLocalSpeakerMgr::LLLocalSpeakerMgr() : LLSpeakerMgr()
{
}

LLLocalSpeakerMgr::~LLLocalSpeakerMgr ()
{
}

void LLLocalSpeakerMgr::updateSpeakerList()
{

    LLSpeakerMgr::updateSpeakerList();

    if (gDisconnected)//the world is cleared.
    {
        return ;
    }


    uuid_vec_t avatar_ids;
    std::vector<LLVector3d> positions;
// <FS:CR> Opensim
    //LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, gAgent.getPositionGlobal(), CHAT_NORMAL_RADIUS);
    LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, gAgent.getPositionGlobal(), (F32)LFSimFeatureHandler::getInstance()->sayRange());
// </FS:CR> Opensim
#ifdef LL_DISCORD
    if (gSavedSettings.getBOOL("EnableDiscord"))
        LLAppViewer::updateDiscordPartyCurrentSize((S32)avatar_ids.size());
#endif
    for(U32 i=0; i<avatar_ids.size(); i++)
    {
        setSpeaker(avatar_ids[i]);
    }

    // check if text only speakers have moved out of chat range
    for (speaker_map_t::iterator speaker_it = mSpeakers.begin(); speaker_it != mSpeakers.end(); ++speaker_it)
    {
        LLUUID speaker_id = speaker_it->first;
        LLPointer<LLSpeaker> speakerp = speaker_it->second;
        if (speakerp.notNull() && speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
        {
            LLVOAvatar* avatarp = (LLVOAvatar*)gObjectList.findObject(speaker_id);
// <FS:CR> Opensim
            F32 say_distance_squared = (F32)(LFSimFeatureHandler::getInstance()->sayRange() * LFSimFeatureHandler::getInstance()->sayRange());
            if (!avatarp || dist_vec_squared(avatarp->getPositionAgent(), gAgent.getPositionAgent()) > say_distance_squared)
            //if (!avatarp || dist_vec_squared(avatarp->getPositionAgent(), gAgent.getPositionAgent()) > CHAT_NORMAL_RADIUS * CHAT_NORMAL_RADIUS)
// </FS:CR> Opensim
            {
                setSpeakerNotInChannel(speakerp);
            }
        }
    }
}
