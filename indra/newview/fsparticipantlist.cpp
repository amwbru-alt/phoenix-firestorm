/**
 * @file fsparticipantlist.cpp
 * @brief FSParticipantList intended to update view(LLAvatarList) according to incoming messages
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llmutelist.h"
#include "fsparticipantlist.h"

// common includes
#include "lltrans.h"
#include "llavataractions.h"
#include "llagent.h"
// [SL:KB] - Patch: Chat-GroupSessionEject | Checked: 2012-02-04 (Catznip-3.2.1)
#include "llgroupactions.h"
// [/SL:KB]
#include "llimview.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llspeakers.h"
#include "llurlaction.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"

//FSParticipantList retrieves add, clear and remove events and updates view accordingly


static void update_speaker_indicator(const LLAvatarList* const avatar_list, const LLUUID& avatar_uuid, bool is_muted)
{
    LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(avatar_list->getItemByValue(avatar_uuid));
    if (item)
    {


    }
}

FSParticipantList::FSParticipantList(LLSpeakerMgr* data_source,
                                     LLAvatarList* avatar_list,
                                     bool use_context_menu/* = true*/,
                                     bool exclude_agent /*= true*/,
                                     bool can_toggle_icons /*= true*/) :
    mSpeakerMgr(data_source),
    mAvatarList(avatar_list),
    mParticipantListMenu(NULL),
    mExcludeAgent(exclude_agent),
    mValidateSpeakerCallback(NULL),
    mInsertMentionCallback(NULL),
    mConvType(CONV_UNKNOWN)
{


    mSpeakerAddListener = new SpeakerAddListener(*this);
    mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
    mSpeakerClearListener = new SpeakerClearListener(*this);
    mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
    mSpeakerMuteListener = new SpeakerMuteListener(*this);

    mSpeakerMgr->addListener(mSpeakerAddListener, "add");
    mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
    mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
    mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");

    mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
    LL_DEBUGS("SpeakingIndicator") << "Set session for speaking indicators: " << mSpeakerMgr->getSessionID() << LL_ENDL;
    mAvatarList->setSessionID(mSpeakerMgr->getSessionID());
    mAvatarListDoubleClickConnection = mAvatarList->setItemDoubleClickCallback(boost::bind(&FSParticipantList::onAvatarListDoubleClicked, this, _1));
    mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&FSParticipantList::onAvatarListRefreshed, this, _1, _2));
    // Set onAvatarListDoubleClicked as default on_return action.
    mAvatarListReturnConnection = mAvatarList->setReturnCallback(boost::bind(&FSParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

    if (use_context_menu)
    {
        mParticipantListMenu = new FSParticipantListMenu(*this);
        mAvatarList->setContextMenu(mParticipantListMenu);
    }
    else
    {
        mAvatarList->setContextMenu(NULL);
    }

    if (use_context_menu && can_toggle_icons)
    {
        mAvatarList->setShowIcons("ParticipantListShowIcons");
        mAvatarListToggleIconsConnection = gSavedSettings.getControl("ParticipantListShowIcons")->getSignal()->connect(boost::bind(&LLAvatarList::toggleIcons, mAvatarList));
    }

    //Lets fill avatarList with existing speakers
    LLSpeakerMgr::speaker_list_t speaker_list;
    mSpeakerMgr->getSpeakerList(&speaker_list, true);
    for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
    {
        const LLPointer<LLSpeaker>& speakerp = *it;

        addAvatarIDExceptAgent(speakerp->mID);
        if ( speakerp->mIsModerator )
        {
            mModeratorList.insert(speakerp->mID);
        }
        else
        {
            mModeratorToRemoveList.insert(speakerp->mID);
        }
    }

    // Identify and store what kind of session we are
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(data_source->getSessionID());
    if (im_session)
    {
        // By default, sessions that can't be identified as group or ad-hoc will be considered P2P (i.e. 1 on 1)
        mConvType = CONV_SESSION_1_ON_1;
        if (im_session->isAdHocSessionType())
        {
            mConvType = CONV_SESSION_AD_HOC;
        }
        else if (im_session->isGroupSessionType())
        {
            mConvType = CONV_SESSION_GROUP;
        }
    }
    else
    {
        // That's the only session that doesn't get listed in the LLIMModel as a session...
        mConvType = CONV_SESSION_NEARBY;
    }

    // we need to exclude agent id for non group chat
    sort();
}

FSParticipantList::~FSParticipantList()
{
    mAvatarListDoubleClickConnection.disconnect();
    mAvatarListRefreshConnection.disconnect();
    mAvatarListReturnConnection.disconnect();
    mAvatarListToggleIconsConnection.disconnect();

    // It is possible Participant List will be re-created from LLCallFloater::onCurrentChannelChanged()
    // See ticket EXT-3427
    // hide menu before deleting it to stop enable and check handlers from triggering.
    if(mParticipantListMenu && !LLApp::isExiting())
    {
        mParticipantListMenu->hide();
    }

    if (mParticipantListMenu)
    {
        delete mParticipantListMenu;
        mParticipantListMenu = NULL;
    }

    mAvatarList->setContextMenu(NULL);
    mAvatarList->setComparator(NULL);
}

;

void FSParticipantList::onAvatarListDoubleClicked(LLUICtrl* ctrl)
{
    LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(ctrl);
    if(!item)
    {
        return;
    }

    LLUUID clicked_id = item->getAvatarId();

    if (clicked_id.isNull() || clicked_id == gAgent.getID())
        return;

    LLAvatarActions::startIM(clicked_id);
}

void FSParticipantList::onAvatarListRefreshed(LLUICtrl* ctrl, const LLSD& param)
{
    LLAvatarList* list = dynamic_cast<LLAvatarList*>(ctrl);
    if (list)
    {
        const std::string moderator_indicator(LLTrans::getString("IM_moderator_label"));
        const std::size_t moderator_indicator_len = moderator_indicator.length();

        // Firstly remove moderators indicator
        std::set<LLUUID>::const_iterator
            moderator_list_it = mModeratorToRemoveList.begin(),
            moderator_list_end = mModeratorToRemoveList.end();
        for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
        {
            LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
            if ( item )
            {
                std::string name = item->getAvatarName();
                std::string tooltip = item->getAvatarToolTip();
                size_t found = name.find(moderator_indicator);
                if (found != std::string::npos)
                {
                    name.erase(found, moderator_indicator_len);
                    item->setAvatarName(name);
                }
                found = tooltip.find(moderator_indicator);
                if (found != tooltip.npos)
                {
                    tooltip.erase(found, moderator_indicator_len);
                    item->setAvatarToolTip(tooltip);
                }
                item->setState(LLAvatarListItem::IS_ONLINE);
            }
        }

        mModeratorToRemoveList.clear();

        // Add moderators indicator
        moderator_list_it = mModeratorList.begin();
        moderator_list_end = mModeratorList.end();
        for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
        {
            LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
            if ( item )
            {
                std::string name = item->getAvatarName();
                std::string tooltip = item->getAvatarToolTip();
                size_t found = name.find(moderator_indicator);
                if (found == std::string::npos)
                {
                    name += " ";
                    name += moderator_indicator;
                    item->setAvatarName(name);
                }
                found = tooltip.find(moderator_indicator);
                if (found == std::string::npos)
                {
                    tooltip += " ";
                    tooltip += moderator_indicator;
                    item->setAvatarToolTip(tooltip);
                }
                item->setState(LLAvatarListItem::IS_GROUPMOD);
            }
        }


        LLSpeakerMgr::speaker_list_t speaker_list;


        mSpeakerMgr->getSpeakerList(&speaker_list, true);
        for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
        {
            const LLPointer<LLSpeaker>& speakerp = *it;

            if (speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
            {

            }
        }
    }
}

void FSParticipantList::setSortOrder(EParticipantSortOrder order)
{
    if (gSavedSettings.getU32("ChatParticipantSortOrder") != order)
    {
        gSavedSettings.setU32("ChatParticipantSortOrder", order);
        sort();
    }
}

const FSParticipantList::EParticipantSortOrder FSParticipantList::getSortOrder() const
{ return EParticipantSortOrder(gSavedSettings.getU32("ChatParticipantSortOrder")); }

void FSParticipantList::setValidateSpeakerCallback(validate_speaker_callback_t cb)
{
    mValidateSpeakerCallback = cb;
}

void FSParticipantList::setInsertMentionCallback(insert_mention_callback_t cb)
{
    mInsertMentionCallback = cb;
}

void FSParticipantList::update()
{
    mSpeakerMgr->update(true);

    if (E_SORT_BY_RECENT_SPEAKERS == getSortOrder() && !isHovered())
    {
        // Resort avatar list
        sort();
    }
}

bool FSParticipantList::isHovered()
{
    S32 x, y;
    LLUI::getInstance()->getMousePositionScreen(&x, &y);
    return mAvatarList->calcScreenRect().pointInRect(x, y);
}

bool FSParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    LLUUID uu_id = event->getValue().asUUID();

    if (mValidateSpeakerCallback && !mValidateSpeakerCallback(uu_id))
    {
        return true;
    }

    addAvatarIDExceptAgent(uu_id);
    sort();
    return true;
}

bool FSParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    uuid_vec_t& group_members = mAvatarList->getIDs();
    uuid_vec_t::iterator pos = std::find(group_members.begin(), group_members.end(), event->getValue().asUUID());
    if(pos != group_members.end())
    {
        group_members.erase(pos);
        mAvatarList->setDirty();
    }
    return true;
}

bool FSParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    uuid_vec_t& group_members = mAvatarList->getIDs();
    group_members.clear();
    mAvatarList->setDirty();
    return true;
}

bool FSParticipantList::onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    const LLSD& evt_data = event->getValue();
    if ( evt_data.has("id") && evt_data.has("is_moderator") )
    {
        LLUUID id = evt_data["id"];
        bool is_moderator = evt_data["is_moderator"];
        if ( id.notNull() )
        {
            if ( is_moderator )
                mModeratorList.insert(id);
            else
            {
                std::set<LLUUID>::iterator it = mModeratorList.find (id);
                if ( it != mModeratorList.end () )
                {
                    mModeratorToRemoveList.insert(id);
                    mModeratorList.erase(id);
                }
            }

            // apply changes immediately
            onAvatarListRefreshed(mAvatarList, LLSD());
        }
    }
    return true;
}

bool FSParticipantList::onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
    if (speakerp.isNull()) return false;

    // update UI on confirmation of moderator mutes

    return true;
}

void FSParticipantList::sort()
{
    if ( !mAvatarList )
        return;

    switch ( getSortOrder() )
    {
        case E_SORT_BY_NAME :
            // if mExcludeAgent == true , then no need to keep agent on top of the list
            if(mExcludeAgent)
            {
                mAvatarList->sortByName();
            }
            else
            {
                mAvatarList->sortByName(true);
            }
            break;
        case E_SORT_BY_RECENT_SPEAKERS:
            if (mSortByRecentSpeakers.isNull())
                mSortByRecentSpeakers = new LLAvatarItemRecentSpeakerComparator(*this);
            mAvatarList->setComparator(mSortByRecentSpeakers.get());
            mAvatarList->sort();
            break;
        default :
            LL_WARNS() << "Unrecognized sort order for " << mAvatarList->getName() << LL_ENDL;
            return;
    }
}

void FSParticipantList::addAvatarIDExceptAgent(const LLUUID& avatar_id)
{
    if (mExcludeAgent && gAgent.getID() == avatar_id) return;
    if (mAvatarList->contains(avatar_id)) return;


    adjustParticipant(avatar_id);
}

void FSParticipantList::adjustParticipant(const LLUUID& speaker_id)
{
    LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
    if (speakerp.isNull()) return;

    // add listener to process moderation changes
    speakerp->addListener(mSpeakerMuteListener);
}

uuid_vec_t FSParticipantList::getAvatarIds() const
{
    if (!mAvatarList)
        return{};

    return uuid_vec_t{ mAvatarList->getIDs() };
}

//
// FSParticipantList::SpeakerAddListener
//
bool FSParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    /**
     * We need to filter speaking objects. These objects shouldn't appear in the list
     * @see LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
     */
    const LLUUID& speaker_id = event->getValue().asUUID();
    LLPointer<LLSpeaker> speaker = mParent.mSpeakerMgr->findSpeaker(speaker_id);
    if(speaker.isNull() || speaker->mType == LLSpeaker::SPEAKER_OBJECT)
    {
        return false;
    }
    return mParent.onAddItemEvent(event, userdata);
}

//
// FSParticipantList::SpeakerRemoveListener
//
bool FSParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    return mParent.onRemoveItemEvent(event, userdata);
}

//
// FSParticipantList::SpeakerClearListener
//
bool FSParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    return mParent.onClearListEvent(event, userdata);
}

//
// FSParticipantList::SpeakerModeratorListener
//
bool FSParticipantList::SpeakerModeratorUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    return mParent.onModeratorUpdateEvent(event, userdata);
}

bool FSParticipantList::SpeakerMuteListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
    return mParent.onSpeakerMuteEvent(event, userdata);
}

LLContextMenu* FSParticipantList::FSParticipantListMenu::createMenu()
{
    // set up the callbacks for all of the avatar menu items
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

    registrar.add("ParticipantList.Sort", boost::bind(&FSParticipantList::FSParticipantListMenu::sortParticipantList, this, _2));
    registrar.add("ParticipantList.AllowTextChat", boost::bind(&FSParticipantList::FSParticipantListMenu::allowTextChat, this, _2));
    registrar.add("ParticipantList.ToggleMuteText", boost::bind(&FSParticipantList::FSParticipantListMenu::toggleMuteText, this, _2));
// [SL:KB] - Patch: Chat-GroupSessionEject | Checked: 2012-02-04 (Catznip-3.2.1) | Added: Catznip-3.2.1
    registrar.add("ParticipantList.Eject", boost::bind(&LLGroupActions::ejectFromGroup, mParent.mSpeakerMgr->getSessionID(), mUUIDs.front()));
// [SL:KB]

    registrar.add("Avatar.Profile", boost::bind(&LLAvatarActions::showProfile, mUUIDs.front()));
    registrar.add("Avatar.IM", boost::bind(&LLAvatarActions::startIM, mUUIDs.front()));
    registrar.add("Avatar.AddFriend", boost::bind(&LLAvatarActions::requestFriendshipDialog, mUUIDs.front()));

    registrar.add("Avatar.Share", boost::bind(&LLAvatarActions::share, mUUIDs.front()));
    registrar.add("Avatar.Pay", boost::bind(&LLAvatarActions::pay, mUUIDs.front()));

    registrar.add("Avatar.AddToContactSet", boost::bind(&FSParticipantList::FSParticipantListMenu::handleAddToContactSet, this));
    registrar.add("Avatar.ZoomIn", boost::bind(&LLAvatarActions::zoomIn, mUUIDs.front()));
    registrar.add("Avatar.BanMember", boost::bind(&FSParticipantList::FSParticipantListMenu::banSelectedMember, this, mUUIDs.front()));


    registrar.add("Mention.CopyURI", boost::bind(&FSParticipantList::FSParticipantListMenu::copyURLToClipboard, this, mUUIDs.front()));
    registrar.add("Mention.Chat", boost::bind(&FSParticipantList::FSParticipantListMenu::insertMentionAtCursor, this, mUUIDs.front()));

    enable_registrar.add("ParticipantList.EnableItem", boost::bind(&FSParticipantList::FSParticipantListMenu::enableContextMenuItem,    this, _2));
    enable_registrar.add("ParticipantList.CheckItem",  boost::bind(&FSParticipantList::FSParticipantListMenu::checkContextMenuItem, this, _2));
    enable_registrar.add("ParticipantList.VisibleItem", boost::bind(&FSParticipantList::FSParticipantListMenu::visibleContextMenuItem, this, _2));

    // create the context menu from the XUI
    LLContextMenu* main_menu = createFromFile("menu_participant_list.xml");

    // Don't show sort options for P2P chat
    bool is_sort_visible = (mParent.mAvatarList && mParent.mAvatarList->size() > 1);
    // <FS:Ansariel> Hide SortBy separator
    main_menu->setItemVisible("Sort Separator", is_sort_visible);
    // </FS:Ansariel>


    main_menu->setItemVisible("SortByName", is_sort_visible);
    main_menu->setItemVisible("SortByRecentSpeakers", is_sort_visible);
    LLPointer<LLSpeaker> moderator = mParent.mSpeakerMgr->findSpeaker(gAgentID);
    const bool is_moderation_visible = moderator.notNull() && moderator->mIsModerator;
    main_menu->setItemVisible("Moderator Options Separator", is_moderation_visible);
    main_menu->setItemVisible("Moderator Options", is_moderation_visible);
    main_menu->setItemVisible("View Icons Separator", mParent.mAvatarListToggleIconsConnection.connected());
    main_menu->setItemVisible("View Icons", mParent.mAvatarListToggleIconsConnection.connected());
    bool show_ban_member = mParent.getType() == FSParticipantList::CONV_SESSION_GROUP && hasAbilityToBan();
    main_menu->setItemVisible("Group Ban Separator", show_ban_member);
    main_menu->setItemVisible("BanMember", show_ban_member);
    main_menu->arrangeAndClear();

    return main_menu;
}

void FSParticipantList::FSParticipantListMenu::copyURLToClipboard(const LLUUID& avatar_id)
{
    LLUrlAction::copyURLToClipboard("fmw:///app/agent/" + mUUIDs.front().asString() + "/mention");
}

void FSParticipantList::FSParticipantListMenu::insertMentionAtCursor(const LLUUID& avatar_id)
{
    if (mParent.mInsertMentionCallback)
    {
        mParent.mInsertMentionCallback(avatar_id);
    }
}



void FSParticipantList::FSParticipantListMenu::sortParticipantList(const LLSD& userdata)
{
    std::string param = userdata.asString();
    if ("sort_by_name" == param)
    {
        mParent.setSortOrder(E_SORT_BY_NAME);
    }
    else if ("sort_by_recent_speakers" == param)
    {
        mParent.setSortOrder(E_SORT_BY_RECENT_SPEAKERS);
    }
}

void FSParticipantList::FSParticipantListMenu::allowTextChat(const LLSD& userdata)
{
    LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(mParent.mSpeakerMgr);
    if (mgr)
    {
        const LLUUID speaker_id = mUUIDs.front();
        mgr->allowTextChat(speaker_id, userdata.asString() == "allow");
    }
}

void FSParticipantList::FSParticipantListMenu::toggleMute(const LLSD& userdata, U32 flags)
{
    const LLUUID speaker_id = mUUIDs.front();
    bool is_muted = LLMuteList::getInstance()->isMuted(speaker_id, flags);
    std::string name;


    LLPointer<LLSpeaker> speakerp = mParent.mSpeakerMgr->findSpeaker(speaker_id);
    if (speakerp.isNull())
    {
        LL_WARNS("Speakers") << "Speaker " << speaker_id << " not found" << LL_ENDL;
        return;
    }

    // We should have the name in the cache from the LLAvatarList this is used in combination with
    LLAvatarName avname;
    if (!LLAvatarNameCache::get(speaker_id, &avname)) return;

    name = avname.getUserName();

    LLMute::EType mute_type;
    switch (speakerp->mType)
    {
        case LLSpeaker::SPEAKER_AGENT:
            mute_type = LLMute::AGENT;
            break;
        case LLSpeaker::SPEAKER_OBJECT:
            mute_type = LLMute::OBJECT;
            break;

        default:
            mute_type = LLMute::EXTERNAL;
            break;
    }
    LLMute mute(speaker_id, name, mute_type);

    if (!is_muted)
    {
        LLMuteList::getInstance()->add(mute, flags);
    }
    else
    {
        LLMuteList::getInstance()->remove(mute, flags);
    }
}

void FSParticipantList::FSParticipantListMenu::toggleMuteText(const LLSD& userdata)
{
    toggleMute(userdata, LLMute::flagTextChat);
}


bool FSParticipantList::FSParticipantListMenu::isGroupModerator()
{
    if (!mParent.mSpeakerMgr)
    {
        LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
        return false;
    }

    // Is this a group text session?
    if(gAgent.isInGroup(mParent.mSpeakerMgr->getSessionID()))
    {
        LLSpeaker* speaker = mParent.mSpeakerMgr->findSpeaker(gAgentID).get();

        // Is agent a moderator?
        return speaker && speaker->mIsModerator;
    }
    return false;
}

bool FSParticipantList::FSParticipantListMenu::hasAbilityToBan()
{
    LLSpeakerMgr * speaker_manager = mParent.mSpeakerMgr;
    if (NULL == speaker_manager)
    {
        LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
        return false;
    }
    LLUUID group_uuid = speaker_manager->getSessionID();

    return gAgent.isInGroup(group_uuid) && gAgent.hasPowerInGroup(group_uuid, GP_GROUP_BAN_ACCESS);
}

bool FSParticipantList::FSParticipantListMenu::canBanSelectedMember(const LLUUID& participant_uuid)
{
    LLSpeakerMgr * speaker_manager = mParent.mSpeakerMgr;
    if (NULL == speaker_manager)
    {
        LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
        return false;
    }
    LLUUID group_uuid = speaker_manager->getSessionID();
    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_uuid);
    if(!gdatap)
    {
        LL_WARNS("Groups") << "Unable to get group data for group " << group_uuid << LL_ENDL;
        return false;
    }

    if (participant_uuid == gAgentID)
    {
        return false;
    }

    if (gdatap->mPendingBanRequest)
    {
        return false;
    }

    if (gdatap->isRoleMemberDataComplete())
    {
        if (gdatap->mMembers.size())
        {
            LLGroupMgrGroupData::member_list_t::iterator mi = gdatap->mMembers.find((participant_uuid));
            if (mi != gdatap->mMembers.end())
            {
                LLGroupMemberData* member_data = (*mi).second;
                // Is the member an owner?
                if (member_data && member_data->isInRole(gdatap->mOwnerRole))
                {
                    return false;
                }
            }
        }
    }

    if( gAgent.hasPowerInGroup(group_uuid, GP_ROLE_REMOVE_MEMBER) &&
        gAgent.hasPowerInGroup(group_uuid, GP_GROUP_BAN_ACCESS) )
    {
        return true;
    }

    return false;
}

void FSParticipantList::FSParticipantListMenu::banSelectedMember(const LLUUID& participant_uuid)
{
    LLSpeakerMgr * speaker_manager = mParent.mSpeakerMgr;
    if (NULL == speaker_manager)
    {
        LL_WARNS() << "Speaker manager is missing" << LL_ENDL;
        return;
    }

    LLUUID group_uuid = speaker_manager->getSessionID();
    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(group_uuid);
    if(!gdatap)
    {
        LL_WARNS("Groups") << "Unable to get group data for group " << group_uuid << LL_ENDL;
        return;
    }
    gdatap->banMemberById(participant_uuid);
}




// static



bool FSParticipantList::FSParticipantListMenu::enableContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
    const LLUUID& participant_id = mUUIDs.front();

    // For now non of "can_view_profile" action and menu actions listed below except "can_block"

    LLPointer<LLSpeaker> participant = mParent.mSpeakerMgr->findSpeaker(mUUIDs.front());
    const bool is_participant_avatar = participant.notNull() && participant->mType == LLSpeaker::SPEAKER_AGENT;
    if (!is_participant_avatar && "can_block" != item) return false;

    if (item == "can_mute_text" || "can_block" == item || "can_share" == item || "can_im" == item
        || "can_pay" == item)
    {
        return mUUIDs.front() != gAgentID;
    }
    else if (item == std::string("can_add"))
    {
        // We can add friends if:
        // - there are selected people
        // - and there are no friends among selection yet.

        bool result = (mUUIDs.size() > 0);

        uuid_vec_t::const_iterator
            id = mUUIDs.begin(),
            uuids_end = mUUIDs.end();

        for (;id != uuids_end; ++id)
        {
            if ( *id == gAgentID || LLAvatarActions::isFriend(*id) )
            {
                result = false;
                break;
            }
        }
        return result;
    }
    else if (item == "can_allow_text_chat")
    {
        return isGroupModerator() && participant_id != gAgentID && is_participant_avatar;
    }
    else if (item == "can_eject")
    {
        return LLGroupActions::canEjectFromGroup(mParent.mSpeakerMgr->getSessionID(), mUUIDs.front());
    }
// [/SL:KB]
    else if (item == "can_zoom_in")
    {
        return LLAvatarActions::canZoomIn(mUUIDs.front());
    }
    else if (item == "can_ban_member")
    {
        return canBanSelectedMember(mUUIDs.front());
    }

    return true;
}

bool FSParticipantList::FSParticipantListMenu::visibleContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
    if (item == "can_allow_text_chat")
    {
        return !isNearbyChatSession();
    }

    return true;
}




bool FSParticipantList::FSParticipantListMenu::checkContextMenuItem(const LLSD& userdata)
{
    std::string item = userdata.asString();
    const LLUUID& id = mUUIDs.front();

    if (item == "is_muted")
    {
        return LLMuteList::getInstance()->isMuted(id, LLMute::flagTextChat);
    }
    else if (item == "is_allowed_text_chat")
    {
        LLPointer<LLSpeaker> selected_speakerp = mParent.mSpeakerMgr->findSpeaker(id);

        if (selected_speakerp.notNull())
        {
            return !selected_speakerp->mModeratorMutedText;
        }
    }
    else if(item == "is_sorted_by_name")
    {
        return E_SORT_BY_NAME == mParent.getSortOrder();
    }
    else if(item == "is_sorted_by_recent_speakers")
    {
        return E_SORT_BY_RECENT_SPEAKERS == mParent.getSortOrder();
    }

    return false;
}

void FSParticipantList::FSParticipantListMenu::handleAddToContactSet()
{
    LLAvatarActions::addToContactSet(mUUIDs);
}

bool FSParticipantList::FSParticipantListMenu::isNearbyChatSession() const
{
    return mParent.mSpeakerMgr->getSessionID().isNull();
}

bool FSParticipantList::LLAvatarItemRecentSpeakerComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
    if (mParent.mSpeakerMgr)
    {
        LLPointer<LLSpeaker> lhs = mParent.mSpeakerMgr->findSpeaker(avatar_item1->getAvatarId());
        LLPointer<LLSpeaker> rhs = mParent.mSpeakerMgr->findSpeaker(avatar_item2->getAvatarId());
        if ( lhs.notNull() && rhs.notNull() )
        {
            // Compare by last speaking time
            if( lhs->mLastSpokeTime != rhs->mLastSpokeTime )
                return ( lhs->mLastSpokeTime > rhs->mLastSpokeTime );
            else if ( lhs->mSortIndex != rhs->mSortIndex )
                return ( lhs->mSortIndex < rhs->mSortIndex );
        }
        else if ( lhs.notNull() )
        {
            // True if only avatar_item1 speaker info available
            return true;
        }
        else if ( rhs.notNull() )
        {
            // False if only avatar_item2 speaker info available
            return false;
        }
    }
    // By default compare by name.
    return LLAvatarItemNameComparator::doCompare(avatar_item1, avatar_item2);
}

//EOF
