/**
 * @file fsgridhandler.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Based on Second Life Viewer Source Code llviewernetwork.cpp
 * Copyright (C) 2010, Linden Research, Inc.
 * With modifications Copyright (C) 2012, arminweatherwax@lavabit.com
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
#include "formegridpolicy.h" // <FS:ForMe> Fixed-grid policy.

#include "llcommandhandler.h"
#include "lllogininstance.h" // to check if logged in yet
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llsecapi.h"
#include "llstartup.h"

#include "lltrans.h"
#include "llweb.h"
#include "llbufferstream.h"
#if LL_WINDOWS
#include <Winsock2.h>
#else
#include <unistd.h>
#endif

#include "fscorehttputil.h"
#include "fspanellogin.h"
#include "lfsimfeaturehandler.h" // <COLOSI opensim multi-currency support />
#include "llmaterialtable.h"     // <FS:Beq> FIRE-31628 for access to static var

void gridDownloadError(const LLSD& aData, LLGridManager* mOwner, GridEntry* mData, LLGridManager::AddState mState)
{
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(aData);

    if (HTTP_GATEWAY_TIME_OUT == status.getType()) // gateway timeout ... well ... retry once >_>
    {
        if (LLGridManager::FETCH == mState)
        {
            mOwner->addGrid(mData, LLGridManager::RETRY);
        }
    }
    else if (status.getType() == HTTP_NOT_MODIFIED && LLGridManager::TRYLEGACY != mState) // not modified
    {
        mOwner->addGrid(mData, LLGridManager::FINISH);
    }
    else if (HTTP_INTERNAL_ERROR == status.getType() && LLGridManager::LOCAL == mState) // add localhost even if its not up
    {
        mOwner->addGrid(mData, LLGridManager::FINISH);
        // since we know now that its not up we cold also start it
    }
    else if (LLGridManager::TRYLEGACY == mState) // we did TRYLEGACY and faild
    {
        if (!mData->grid.has("FLAG_TEMPORARY"))
        {
            LLSD args;
            args["GRID"] = mData->grid[GRID_VALUE];
            // Could not add [GRID] to the grid list.
            std::string reason_dialog = "Server didn't provide grid info: ";
            reason_dialog.append(mData->last_http_error);
            reason_dialog.append("\nPlease check if the loginuri is correct and");
            args["REASON"] = reason_dialog;
            //[REASON] contact support of [GRID].
            LLNotificationsUtil::add("CantAddGrid", args);
        }

        LL_WARNS() << "No legacy login page. Giving up for " << mData->grid[GRID_VALUE] << LL_ENDL;
        mOwner->addGrid(mData, LLGridManager::FAIL);
    }
    else
    {
        // remember the error we got when trying to get grid info where we expect it
        std::ostringstream last_error;
        last_error << status.getType() << " " << status.getMessage();
        mData->last_http_error = last_error.str();

        mOwner->addGrid(mData, LLGridManager::TRYLEGACY);
    }
}

void gridDownloadComplete(const LLSD& aData, LLGridManager* mOwner, GridEntry* mData, LLGridManager::AddState mState)
{
    // mOwner->decResponderCount();
    LLSD header = aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS][LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
    LLCore::HttpStatus status =
        LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);

    const LLSD::Binary& rawData = aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();

    // LL_DEBUGS("GridManager") << mData->grid[GRID_VALUE] << " status: " << getStatus() << " reason: " << getReason() << LL_ENDL;
    if (LLGridManager::TRYLEGACY == mState && HTTP_OK == status.getType())
    {
        mOwner->addGrid(mData, LLGridManager::SYSTEM);
    }
    else if (HTTP_OK == status.getType()) // OK
    {
        LL_DEBUGS("GridManager") << "Parsing gridinfo xml file from " << mData->grid[GRID_VALUE] << LL_ENDL;

        std::string stringData;
        stringData.assign(rawData.begin(),
                          rawData.end()); // LLXMLNode::parseBuffer wants a U8*, not a const U8*, so need to copy here just to be safe
        if (LLXMLNode::parseBuffer(reinterpret_cast<char*>(&stringData[0]), (U64)stringData.size(), mData->info_root, NULL))
        {
            mOwner->gridInfoResponderCB(mData);
        }
        else
        {
            LLSD args;
            args["GRID"] = mData->grid[GRID_VALUE];
            // Could not add [GRID] to the grid list.
            args["REASON"] = "Server provided broken grid info xml. Please";
            //[REASON] contact support of [GRID].
            LLNotificationsUtil::add("CantAddGrid", args);

            LL_WARNS() << " Could not parse grid info xml from server." << mData->grid[GRID_VALUE] << " skipping." << LL_ENDL;
            mOwner->addGrid(mData, LLGridManager::FAIL);
        }
    }
    else if (HTTP_INTERNAL_ERROR == status.getType() && LLGridManager::LOCAL == mState) // add localhost even if its not up
    {
        mOwner->addGrid(mData, LLGridManager::FINISH);
        // since we know now that its not up we cold also start it
    }
    else
    {
        gridDownloadError(aData[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS], mOwner, mData, mState);
    }
}

const char* DEFAULT_LOGIN_PAGE = "https://phoenixviewer.com/app/loginV3/";

const char* SYSTEM_GRID_SLURL_BASE     = "secondlife://%s/secondlife/";
const char* MAIN_GRID_SLURL_BASE       = "https://maps.secondlife.com/secondlife/";
const char* SYSTEM_GRID_APP_SLURL_BASE = "secondlife:///app";

const char* DEFAULT_HOP_BASE       = "hop://%s/"; // <AW: hop:// protocol>
const char* DEFAULT_SLURL_BASE     = "https://%s/region/";
const char* DEFAULT_APP_SLURL_BASE = "x-grid-location-info://%s/app";

// <AW opensim>
LLGridManager::LLGridManager() : EGridPlatform(GP_NOTSET), mReadyToLogin(false), mCommandLineDone(false), mResponderCount(0), mClassifiedFee(0), mDirectoryFee(0)
{
    mGrid.clear();
}

void LLGridManager::resetGrids()
{
    initGrids();
}

void LLGridManager::initGrids()
{
    // <FS:ForMe> Construct trusted data directly; never read cached/downloaded/user grids.
    mGridList = LLSD::emptyMap();
    LLSD grid = LLSD::emptyMap();
    grid[GRID_VALUE] = ForMeGrid::GridID;
    grid[GRID_ID_VALUE] = ForMeGrid::GridID;
    grid[GRID_LABEL_VALUE] = ForMeGrid::Label;
    grid[GRID_NICK_VALUE] = "forme";
    grid[GRID_PLATFORM] = "OpenSim";
    grid[GRID_REGISTER_NEW_ACCOUNT] = "http://185.173.93.200:8002/web/register.html";
    grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
    grid[GRID_LOGIN_URI_VALUE].append(ForMeGrid::LoginURI);
    grid[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
    grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);
    grid[GRID_LOGIN_PAGE_VALUE] = "http://185.173.93.200:8002/web/index.html";
    grid[GRID_SLURL_BASE] = "hop://185.173.93.200:8002/";
    grid[GRID_APP_SLURL_BASE] = "x-grid-location-info://185.173.93.200:8002/app";
    grid[GRID_IS_SYSTEM_GRID_VALUE] = true;
    grid[GRID_IS_FAVORITE_VALUE] = true;
    mGridList[ForMeGrid::GridID] = grid;
    mGrid = ForMeGrid::GridID;
    mConnectedGrid = grid;
    mGridFile.clear();
    mStartupGrid.clear();
    mCommandLineDone = true;
    mReadyToLogin = true;
    EGridPlatform = GP_OPENSIM;
    gSavedSettings.setString("CurrentGrid", ForMeGrid::GridID);
    gSavedSettings.setString("CmdLineGridChoice", "");
    gSavedSettings.setBOOL("ForceShowGrid", false);
    gSavedSettings.setBOOL("FSOpenSimAlwaysForceShowGrid", false);
    LLTrans::setDefaultArg("CURRENT_GRID", ForMeGrid::Label);
    // </FS:ForMe>
}

void LLGridManager::initSystemGrids()
{
    initGrids();
}

void LLGridManager::initGridList(std::string grid_file, AddState state)
{
    // <FS:ForMe> External grid catalogs cannot alter the fixed grid.
}

void LLGridManager::initCmdLineGrids()
{
    mCommandLineDone = true;
    setGridChoice(ForMeGrid::GridID);
}

LLGridManager::~LLGridManager()
{
}

void LLGridManager::getGridData(const std::string& grid, LLSD& grid_info)
{
    grid_info = ForMeGrid::allowedGrid(grid) ? mGridList[ForMeGrid::GridID] : LLSD();
}

void LLGridManager::gridInfoResponderCB(GridEntry* grid_entry)
{
    for (LLXMLNode* node = grid_entry->info_root->getFirstChild(); node != NULL; node = node->getNextSibling())
    {
        std::string check;
        check = "login";
        if (node->hasName(check))
        {
            // // allow redirect but not spoofing
            // LLURI uri (node->getTextContents());
            // std::string authority = uri.authority();
            // if(! authority.empty())
            // {
            //  grid_entry->grid[GRID_VALUE] = authority;
            // }

            grid_entry->grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
            grid_entry->grid[GRID_LOGIN_URI_VALUE].append(node->getTextContents());
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_LOGIN_URI_VALUE] << LL_ENDL;
            continue;
        }
        check = "gridname";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_LABEL_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_LABEL_VALUE] << LL_ENDL;
            continue;
        }
        check = "gridnick";
        if (node->hasName(check))
        {
            grid_entry->grid[check] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = "gatekeeper";
        if (node->hasName(check))
        {
            LLURI gatekeeper(node->getTextContents());
            grid_entry->grid[check] = gatekeeper.authority();

            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = "welcome";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_LOGIN_PAGE_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_LOGIN_PAGE_VALUE] << LL_ENDL;
            continue;
        }
        check = GRID_REGISTER_NEW_ACCOUNT;
        if (node->hasName(check))
        {
            grid_entry->grid[check] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = GRID_FORGOT_PASSWORD;
        if (node->hasName(check))
        {
            grid_entry->grid[check] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = "help";
        if (node->hasName(check))
        {
            grid_entry->grid[check] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = "about";
        if (node->hasName(check))
        {
            grid_entry->grid[check] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[check] << LL_ENDL;
            continue;
        }
        check = "search";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_SEARCH] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_SEARCH] << LL_ENDL;
            continue;
        }
        check = "web_profile_url";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_WEB_PROFILE_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_WEB_PROFILE_VALUE] << LL_ENDL;
            continue;
        }
        check = "profileuri";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_WEB_PROFILE_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_WEB_PROFILE_VALUE] << LL_ENDL;
            continue;
        }
        check = "SendGridInfoToViewerOnLogin";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_SENDGRIDINFO] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_SENDGRIDINFO] << LL_ENDL;
            continue;
        }
        check = "DirectoryFee";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_DIRECTORY_FEE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_DIRECTORY_FEE] << LL_ENDL;
            continue;
        }
        check = "platform";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_PLATFORM] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_PLATFORM] << LL_ENDL;
            continue;
        }
        check = "message";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_MESSAGE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_MESSAGE] << LL_ENDL;
            continue;
        }
        check = "helperuri";
        if (node->hasName(check))
        {
            grid_entry->grid[GRID_HELPER_URI_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_HELPER_URI_VALUE] << LL_ENDL;
            // don't continue, also check the next
        }
        check = "economy";
        if (node->hasName(check))
        { // sic! economy and helperuri is 2 names for the same
            grid_entry->grid[GRID_HELPER_URI_VALUE] = node->getTextContents();
            LL_DEBUGS("GridManager") << "[\"" << check << "\"]: " << grid_entry->grid[GRID_HELPER_URI_VALUE] << LL_ENDL;
        }
    }

    std::string grid = grid_entry->grid[GRID_VALUE].asString();
    std::string slurl_base(llformat(DEFAULT_HOP_BASE, grid.c_str())); // <AW: hop:// protocol>
    grid_entry->grid[GRID_SLURL_BASE] = slurl_base;

    LLDate now                       = LLDate::now();
    grid_entry->grid["LastModified"] = now;

    addGrid(grid_entry, FINISH);
}

void LLGridManager::addGrid(const std::string& loginuri)
{
    mGridListChangedSignal(false); // <FS:ForMe> Grid additions are disabled.
}

/// LLGridManager::addGrid - add a grid to the grid list, populating the needed values
/// if they're not populated yet.
void LLGridManager::addGrid(GridEntry* grid_entry, AddState state)
{
    delete grid_entry;
    mGridListChangedSignal(false); // <FS:ForMe> Never fetch grid-info for supplied URLs.
}

//
// LLGridManager::addSystemGrid - helper for adding a system grid.
void LLGridManager::addSystemGrid(const std::string& label,
                                  const std::string& name,
                                  const std::string& nick,
                                  const std::string& login_uri,
                                  const std::string& helper,
                                  const std::string& login_page)
{
    GridEntry* grid_entry                   = new GridEntry;
    grid_entry->set_current                 = false;
    grid_entry->grid                        = LLSD::emptyMap();
    grid_entry->grid[GRID_VALUE]            = name;
    grid_entry->grid[GRID_LABEL_VALUE]      = label;
    grid_entry->grid[GRID_NICK_VALUE]       = nick;
    grid_entry->grid[GRID_HELPER_URI_VALUE] = helper;
    grid_entry->grid[GRID_LOGIN_URI_VALUE]  = LLSD::emptyArray();
    grid_entry->grid[GRID_LOGIN_URI_VALUE].append(login_uri);
    grid_entry->grid[GRID_LOGIN_PAGE_VALUE]       = login_page;
    grid_entry->grid[GRID_IS_SYSTEM_GRID_VALUE]   = true;
    grid_entry->grid[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
    grid_entry->grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);

    grid_entry->grid[GRID_APP_SLURL_BASE] = SYSTEM_GRID_APP_SLURL_BASE;

    // only add the system grids beyond agni to the visible list
    // if we're building a debug version.
    if (name == std::string(MAINGRID))
    {
        grid_entry->grid[GRID_SLURL_BASE]        = MAIN_GRID_SLURL_BASE;
        grid_entry->grid[GRID_IS_FAVORITE_VALUE] = true;
    }
    else
    {
        grid_entry->grid[GRID_SLURL_BASE] = llformat(SYSTEM_GRID_SLURL_BASE, label.c_str());
    }

    LL_DEBUGS("PanelLogin") << " " << grid_entry->grid[GRID_LOGIN_PAGE_VALUE] << LL_ENDL;

    try
    {
        addGrid(grid_entry, SYSTEM);
    }
    catch (LLInvalidGridName ex)
    {
    }
}

void LLGridManager::reFetchGrid(const std::string& grid, bool set_current)
{
    // <FS:ForMe> Remote grid-info cannot replace login endpoints.
}

void LLGridManager::removeGrid(const std::string& grid)
{
    // <FS:ForMe> The only grid cannot be removed.
}

boost::signals2::connection LLGridManager::addGridListChangedCallback(grid_list_changed_callback_t cb)
{
    return mGridListChangedSignal.connect(cb);
}

// return a list of grid name -> grid label mappings for UI purposes
std::map<std::string, std::string> LLGridManager::getKnownGrids()
{
    return {{ForMeGrid::GridID, ForMeGrid::Label}};
}

std::string LLGridManager::getGrid(const std::string& grid) const
{
    return ForMeGrid::allowedGrid(grid) ? ForMeGrid::GridID : "";
}

std::string LLGridManager::getGridLabel(const std::string& grid)
{
    std::string grid_label;
    std::string grid_name = getGrid(grid);
    if (!grid_name.empty())
    {
        grid_label = mGridList[grid_name][GRID_LABEL_VALUE].asString();
    }
    else
    {
        if (LLStartUp::getStartupState() >= STATE_LOGIN_SHOW)
        {
            LL_WARNS("GridManager") << "invalid grid '" << grid << "'" << LL_ENDL;
        }
    }
    LL_DEBUGS("GridManager") << "returning " << grid_label << LL_ENDL;
    return grid_label;
}

void LLGridManager::setGridChoice(const std::string& grid)
{
    if (!ForMeGrid::allowedGrid(grid))
    {
        LL_WARNS("GridManager") << "This viewer connects only to ForMe.World." << LL_ENDL;
        return;
    }
    mGrid = ForMeGrid::GridID;
    gSavedSettings.setString("CurrentGrid", ForMeGrid::GridID);
    EGridPlatform = GP_OPENSIM;
    mReadyToLogin = true;
}

std::string LLGridManager::getGridByProbing(const std::string& probe_for, bool case_sensitive) const
{
    std::string ret;
    ret = getGridByHostName(probe_for, case_sensitive);
    if (ret.empty())
    {
        getGridByGridNick(probe_for, case_sensitive);
    }
    if (ret.empty())
    {
        getGridByLabel(probe_for, case_sensitive);
    }

    return ret;
}

std::string LLGridManager::getGridByLabel(const std::string& grid_label, bool case_sensitive) const
{
    return grid_label.empty() ? std::string() : getGridByAttribute(GRID_LABEL_VALUE, grid_label, case_sensitive);
}

std::string LLGridManager::getGridByGridNick(const std::string& grid_nick, bool case_sensitive) const
{
    return grid_nick.empty() ? std::string() : getGridByAttribute(GRID_NICK_VALUE, grid_nick, case_sensitive);
}

std::string LLGridManager::getGridByHostName(const std::string& host_name, bool case_sensitive) const
{
    return host_name.empty() ? std::string() : getGridByAttribute(GRID_VALUE, host_name, case_sensitive);
}

std::string LLGridManager::getGridByAttribute(const std::string& attribute, const std::string& attribute_value, bool case_sensitive) const
{
    if (attribute.empty() || attribute_value.empty())
    {
        return std::string();
    }

    for (LLSD::map_const_iterator grid_iter = mGridList.beginMap(); grid_iter != mGridList.endMap(); grid_iter++)
    {
        if (grid_iter->second.has(attribute))
        {
            if (0 == (case_sensitive ? LLStringUtil::compareStrings(attribute_value, grid_iter->second[attribute].asString())
                                     : LLStringUtil::compareInsensitive(attribute_value, grid_iter->second[attribute].asString())))
            {
                return grid_iter->first;
            }
        }
    }
    return std::string();
}

std::string LLGridManager::getGridId(const std::string& grid)
{
    std::string grid_id;
    std::string grid_name = getGrid(grid);
    if (!grid_name.empty())
    {
        // FS:AW FIRE-7468
        // NOTE: GRID_ID_VALUE  has a different meaning in llviewernetwork
        // than here, which was inherited from the pathfinding changes.
        // GRID_NICK_VALUE is the one we want *here*.
        grid_id = mGridList[grid_name][GRID_NICK_VALUE].asString();
    }
    else
    {
        if (LLStartUp::getStartupState() >= STATE_LOGIN_SHOW)
        {
            LL_WARNS("GridManager") << "invalid grid '" << grid << "'" << LL_ENDL;
        }
    }
    LL_DEBUGS("GridManager") << "returning " << grid_id << LL_ENDL;
    return grid_id;
}

// this assumes that there is anyway only one uri saved
std::string LLGridManager::getLoginURI(const std::string& grid)
{
    return ForMeGrid::allowedGrid(grid) ? ForMeGrid::LoginURI : "";
}

void LLGridManager::getLoginURIs(std::vector<std::string>& uris)
{
    uris.assign(1, ForMeGrid::LoginURI);
}

std::string LLGridManager::getHelperURI()
{
    const std::string helper = LFSimFeatureHandler::getInstance()->helperUriOverride();
    return ForMeGrid::allowedServiceURI(helper) ? helper : ForMeGrid::LoginURI;
}

std::string LLGridManager::getLoginPage()
{
    return "http://185.173.93.200:8002/web/index.html";
}

std::string LLGridManager::getWebProfileURL(const std::string& grid)
{
    std::string web_profile_url;
    std::string grid_name = getGrid(grid);
    if (!grid_name.empty())
    {
        web_profile_url = mGridList[grid_name][GRID_WEB_PROFILE_VALUE].asString();
    }
    else
    {
        LL_WARNS("GridManager") << "invalid grid '" << grid << "'" << LL_ENDL;
    }
    return web_profile_url;
}

void LLGridManager::updateIsInProductionGrid()
{
    EGridPlatform = GP_OPENSIM;
}

// For any Second Life grid
bool LLGridManager::isInSecondLife() const
{
    return (EGridPlatform == GP_SLMAIN || EGridPlatform == GP_SLBETA);
}

// For Agni
bool LLGridManager::isInSLMain() const
{
    return (EGridPlatform == GP_SLMAIN);
}

// For Aditi
bool LLGridManager::isInSLBeta() const
{
    return (EGridPlatform == GP_SLBETA);
}

// For OpenSim
bool LLGridManager::isInOpenSim() const
{
    return (EGridPlatform == GP_OPENSIM || EGridPlatform == GP_AURORA);
}

// For Aurora Sim
bool LLGridManager::isInAuroraSim() const
{
    return (EGridPlatform == GP_AURORA);
}

void LLGridManager::saveGridList()
{
    // filter out just those which are not hardcoded anyway
    LLSD output_grid_list = LLSD::emptyMap();
    if (mGridList[mGrid].has("FLAG_TEMPORARY"))
    {
        mGridList[mGrid].erase("FLAG_TEMPORARY");
    }
    for (LLSD::map_iterator grid_iter = mGridList.beginMap(); grid_iter != mGridList.endMap(); grid_iter++)
    {
        if (!(grid_iter->first.empty() || grid_iter->second.isUndefined() || grid_iter->second.has("FLAG_TEMPORARY") ||
              grid_iter->second.has("DEPRECATED")))
        {
            output_grid_list[grid_iter->first] = grid_iter->second;
        }
    }
    llofstream llsd_xml;
    llsd_xml.open(mGridFile.c_str(), std::ios::out | std::ios::binary);
    LLSDSerialize::toPrettyXML(output_grid_list, llsd_xml);
    llsd_xml.close();
}

//<AW opensim>
std::string LLGridManager::trimHypergrid(const std::string& trim)
{
    std::size_t pos;
    std::string grid = trim;

    pos = grid.find_last_of(":");
    if (pos != std::string::npos)
    {
        std::string part = grid.substr(pos + 1, grid.length() - 1);
        // in hope numbers only is a good guess for it's a port number
        if (std::string::npos != part.find_first_not_of("1234567890/"))
        {
            // and erase if not
            grid.erase(pos, grid.length() - 1);
        }
    }

    return grid;
}
//</AW opensim>

// get location slurl base for the given region
// within the selected grid (LL comment was misleading)
std::string LLGridManager::getSLURLBase(const std::string& grid)
{
    std::string grid_base;
    std::string ret;

    std::string grid_trimmed = trimHypergrid(grid);

    if (mGridList.has(grid_trimmed) && mGridList[grid_trimmed].has(GRID_SLURL_BASE))
    {
        ret = mGridList[grid_trimmed][GRID_SLURL_BASE].asString();
        LL_DEBUGS("GridManager") << "GRID_SLURL_BASE: " << ret << LL_ENDL; // <AW opensim>
    }
    //<AW opensim>
    else
    {
        if (!mFetchedGridInfo.contains(grid))
        {
            LL_DEBUGS("GridManager") << "Trying to fetch info for:" << grid << LL_ENDL;
            GridEntry* grid_entry        = new GridEntry;
            grid_entry->set_current      = false;
            grid_entry->grid             = LLSD::emptyMap();
            grid_entry->grid[GRID_VALUE] = grid;
            mFetchedGridInfo.insert(grid);

            // add the grid with the additional values, or update the
            // existing grid if it exists with the given values
            addGrid(grid_entry, FETCHTEMP);
        }

        // deal with hand edited entries
        std::string grid_norm = grid;

        if (!grid_norm.empty())
        {
            size_t pos = grid_norm.find_last_of("/");

            if ((grid_norm.length() - 1) == pos)
                grid_norm.erase(pos);
        }

        ret = llformat(DEFAULT_HOP_BASE, grid_norm.c_str());                // <AW: hop:// protocol>
        LL_DEBUGS("GridManager") << "DEFAULT_HOP_BASE: " << ret << LL_ENDL; // <AW opensim>
                                                                            // </AW opensim>
    }

    return ret;
}

std::string LLGridManager::getUpdateServiceURL()
{
    return ""; // <FS:ForMe> Do not replace this distribution with unrestricted upstream builds.
}

// get app slurl base for the given region
// within the selected grid (LL comment was misleading)
std::string LLGridManager::getAppSLURLBase(const std::string& grid)
{
    std::string grid_base;
    std::string ret;

    if (mGridList.has(grid) && mGridList[grid].has(GRID_APP_SLURL_BASE))
    {
        ret = mGridList[grid][GRID_APP_SLURL_BASE].asString();
    }
    else
    {
        std::string app_base;
        if (mGridList.has(grid) && mGridList[grid].has(GRID_SLURL_BASE))
        {
            std::string grid_slurl_base = mGridList[grid][GRID_SLURL_BASE].asString();
            if (0 == grid_slurl_base.find("hop://"))
            {
                app_base = DEFAULT_HOP_BASE;
                app_base.append("app");
            }
            else
            {
                app_base = DEFAULT_APP_SLURL_BASE;
            }
        }
        else
        {
            app_base = DEFAULT_APP_SLURL_BASE;
        }

        // deal with hand edited entries
        std::string grid_norm = grid;
        size_t      pos       = grid_norm.find_last_of("/");
        if ((grid_norm.length() - 1) == pos)
        {
            grid_norm.erase(pos);
        }
        ret = llformat(app_base.c_str(), grid_norm.c_str());
    }

    LL_DEBUGS("GridManager") << "App slurl base: " << ret << " - grid = " << grid << LL_ENDL;
    return ret;
}

class FSGridManagerCommandHandler : public LLCommandHandler
{
public:
    FSGridManagerCommandHandler() : LLCommandHandler("gridmanager", UNTRUSTED_THROTTLE), mDownloadConnection() {}

    ~FSGridManagerCommandHandler()
    {
        if (mDownloadConnection.connected())
        {
            mDownloadConnection.disconnect();
        }
    }

    bool handle(const LLSD& params, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web)
    {
        if (params.size() < 2)
        {
            return false;
        }

        // Automatically add and select grid via secondlife:///app/gridmanager/addgrid/<URL-encoded login URI>
        // Example: secondlife:///app/gridmanager/addgrid/http%3A%2F%2Fgrid.avatarlife.com%3A8002
        if (params[0].asString() == "addgrid")
        {
            std::string login_uri = LLURI::unescape(params[1].asString());
            mDownloadConnection   = LLGridManager::instance().addGridListChangedCallback(
                boost::bind(&FSGridManagerCommandHandler::handleGridDownloadComplete, this, _1));
            LLGridManager::instance().addGrid(login_uri);
            return true;
        }

        return false;
    }

private:
    boost::signals2::connection mDownloadConnection;

    void handleGridDownloadComplete(bool success)
    {
        if (mDownloadConnection.connected())
        {
            mDownloadConnection.disconnect();
        }

        if (success)
        {
            LLGridManager::getInstance()->saveGridList();
            if (LLStartUp::getStartupState() <= STATE_LOGIN_WAIT)
            {
                FSPanelLogin::updateServer();
            }
        }
    }
};

// Creating the object registers with the dispatcher.
FSGridManagerCommandHandler gFSGridManagerCommandHandler;
