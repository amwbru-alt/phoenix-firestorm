/**
 * @file fsslurl.cpp (was llsimurlstring.cpp)
 * @brief Handles "SLURL fragments" like Ahern/123/45 for
 * startup processing, login screen, prefs, etc.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Based on Second Life Viewer Source Code llslurl.cpp
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA 94111 USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llslurl.h"
#include "formeurlpolicy.h"

#include "llpanellogin.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "curl/curl.h"
#include "llstartup.h"
// [RLVa:KB] - Checked: 2010-04-05 (RLVa-1.2.0d)
#include "rlvhandler.h"
// [/RLVa:KB]
const char* LLSLURL::HOP_SCHEME      = "fmw"; // <AW: fmw:// protocol>
const char* LLSLURL::SLURL_HTTP_SCHEME       = "http";
const char* LLSLURL::SLURL_HTTPS_SCHEME      = "https";
const char* LLSLURL::SLURL_SECONDLIFE_SCHEME     = "fmw";
const char* LLSLURL::SLURL_X_GRID_LOCATION_INFO_SCHEME = "fmw";

// For DnD - even though www.slurl.com redirects to slurl.com in a browser, you  can copy and drag
// text with www.slurl.com or a link explicitly pointing at www.slurl.com so testing for this
// version is required also.

const char* LLSLURL::WWW_SLURL_COM               = "www.slurl.com";
const char* LLSLURL::MAPS_SECONDLIFE_COM         = "maps.secondlife.com";

const char* LLSLURL::SLURL_COM                   = "slurl.com";
const char* LLSLURL::SLURL_APP_PATH              = "app";
const char* LLSLURL::SLURL_SECONDLIFE_PATH       = "fmw";
const char* LLSLURL::SLURL_REGION_PATH           = "region";
const char* LLSLURL::SIM_LOCATION_HOME           = "home";
const char* LLSLURL::SIM_LOCATION_LAST           = "last";

// resolve a simstring from a slurl
LLSLURL::LLSLURL(const std::string& slurl)
: mHypergrid(false)
{
    mType = INVALID;
    mGrid = ForMeGrid::GridID;
    if (slurl.empty() || slurl == SIM_LOCATION_LAST) { mType = LAST_LOCATION; return; }
    if (slurl == SIM_LOCATION_HOME) { mType = HOME_LOCATION; return; }
    const std::string input = slurl.find("://") == std::string::npos
        ? std::string("fmw://") + slurl : slurl;
    const ForMeURL::Link link = ForMeURL::parse(input);
    if (!link.valid) return;
    if (link.app)
    {
        mType = APP;
        mAppCmd = link.path.front();
        mAppPath = LLSD::emptyArray();
        for (std::size_t i = 1; i < link.path.size(); ++i) mAppPath.append(link.path[i]);
        mAppQuery = link.query;
        mAppQueryMap = LLURI(input).queryMap();
    }
    else
    {
        mType = LOCATION;
        mRegion = link.region;
        mPosition = LLVector3(link.position[0], link.position[1], link.position[2]);
    }
}


// Create a slurl for the middle of the region
LLSLURL::LLSLURL(const std::string& grid, const std::string& region, bool hypergrid)
: mHypergrid(hypergrid)
{
    mGrid = grid;
    mRegion = region;
    mType = LOCATION;
    mPosition = LLVector3((F64)REGION_WIDTH_METERS/2, (F64)REGION_WIDTH_METERS/2, 0);
}

// create a slurl given the position.  The position will be modded with the region
// width handling global positions as well
LLSLURL::LLSLURL(const std::string& grid, const std::string& region, const LLVector3& position, bool hypergrid)
: mHypergrid(hypergrid)
{
    mGrid = grid;
    mRegion = region;
// <FS:CR> FIRE-8063 - Aurora sim var region teleports
    //S32 x = ll_round( (F32)fmod( position[VX], (F32)REGION_WIDTH_METERS ) );
    //S32 y = ll_round( (F32)fmod( position[VY], (F32)REGION_WIDTH_METERS ) );
    //S32 z = ll_round( (F32)position[VZ] );
    mPosition = position;
// </FS:CR>
    mType = LOCATION;
// <FS:CR> FIRE-8063 - Aurora sim var region teleports
    //mPosition = LLVector3((F32)x, (F32)y, (F32)z);

    if(!LLGridManager::getInstance()->isInOpenSim())
    {
        S32 x = ll_round( (F32)fmod( position[VX], (F32)REGION_WIDTH_METERS ) );
        S32 y = ll_round( (F32)fmod( position[VY], (F32)REGION_WIDTH_METERS ) );
        S32 z = ll_round( (F32)position[VZ] );
        mPosition = LLVector3((F32)x, (F32)y, (F32)z);
    }
// </FS:CR>
}

// create a simstring
LLSLURL::LLSLURL(const std::string& region, const LLVector3& position, bool hypergrid)
: mHypergrid(hypergrid)
{
    *this = LLSLURL(LLGridManager::getInstance()->getGrid(), region, position);
}

// <FS:Beq pp Oren> FIRE-30768: SLURL's don't work in VarRegions
// create a slurl from a global position
//LLSLURL::LLSLURL(const std::string& grid, const std::string& region, const LLVector3d& global_position, bool hypergrid)
//: mHypergrid(hypergrid)
//{
// <FS:CR> Aurora-sim var region teleports
    //*this = LLSLURL(grid,
    //    region, LLVector3(global_position.mdV[VX],
    //              global_position.mdV[VY],
    //              global_position.mdV[VZ]));
//  S32 x = ll_round( (F32)fmod( (F32)global_position.mdV[VX], (F32)REGION_WIDTH_METERS ) );
//  S32 y = ll_round( (F32)fmod( (F32)global_position.mdV[VY], (F32)REGION_WIDTH_METERS ) );
//  S32 z = ll_round( (F32)global_position.mdV[VZ] );

//  *this = LLSLURL(grid, region, LLVector3((F32)x, (F32)y, (F32)z));
// </FS:CR>
//}
//
// create a slurl from a global position
//LLSLURL::LLSLURL(const std::string& region, const LLVector3d& global_position, bool hypergrid)
//: mHypergrid(hypergrid)
//{
//  *this = LLSLURL(LLGridManager::getInstance()->getGrid(), region, global_position);
//}

// create a slurl from a global position
LLSLURL::LLSLURL(const std::string& grid, const std::string& region, const LLVector3d& region_origin, const LLVector3d& global_position, bool hypergrid)
    : mHypergrid(hypergrid)
{
    LLVector3 local_position = LLVector3(global_position - region_origin);
    *this = LLSLURL(grid, region, local_position);
}

// create a slurl from a global position
LLSLURL::LLSLURL(const std::string& region, const LLVector3d& region_origin, const LLVector3d& global_position, bool hypergrid)
    : mHypergrid(hypergrid)
{
    *this = LLSLURL(LLGridManager::getInstance()->getGrid(), region, region_origin, global_position);
}
// </FS:Beq pp Oren>

LLSLURL::LLSLURL(const std::string& command, const LLUUID&id, const std::string& verb)
: mHypergrid(false)
{
    mType = APP;
    mAppCmd = command;
    mAppPath = LLSD::emptyArray();
    mAppPath.append(LLSD(id));
    mAppPath.append(LLSD(verb));
}

//<AW: opensim>
LLSLURL::LLSLURL(const LLSD& path_array, bool from_app)
: mHypergrid(false)
{
    if (path_array.isArray() && path_array.size() > 0)
    {
        std::string query="fmw://";
        for(int i=0; path_array.size()>i; i++)
        {
            query += path_array[i].asString() + "/";
        }

        LLSLURL result(query);
        if(APP == result.getType() && from_app)
        {
            mType = INVALID;
        }
        else
        {
            *this = result;
        }
    }
    else
    {
        mType = INVALID;
    }
}
//</AW: opensim>

std::string LLSLURL::getSLURLString() const
{
    switch(mType)
    {
    case HOME_LOCATION:
        return SIM_LOCATION_HOME;
    case LAST_LOCATION:
        return SIM_LOCATION_LAST;
    case LOCATION:
    {
        // lookup the grid
        S32 x = ll_round( (F32)mPosition[VX] );
        S32 y = ll_round( (F32)mPosition[VY] );
        S32 z = ll_round( (F32)mPosition[VZ] );
        std::string ret = LLGridManager::getInstance()->getSLURLBase(mGrid);
//              ret.append(LLURI::escape(mRegion));
//              ret.append(llformat("/%d/%d/%d",x,y,z));
// [RLVa:KB] - Checked: 2010-04-05 (RLVa-1.2.0d) | Added: RLVa-1.2.0d
        ret.append( ( ((!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) || (!RlvUtil::isNearbyRegion(mRegion)))
                     ? (LLURI::escape(mRegion) + llformat("/%d/%d/%d",x,y,z))
                     : RlvStrings::getString(RlvStringKeys::Hidden::Region) ));
// [/RLVa:KB]
        LL_DEBUGS("SLURL") << "Location: " << ret << LL_ENDL;
        return ret;

    }

    case APP:
    {
        std::ostringstream app_url;
        app_url << LLGridManager::getInstance()->getAppSLURLBase() << "/" << mAppCmd;
        for(LLSD::array_const_iterator i = mAppPath.beginArray();
                i != mAppPath.endArray();
                i++)
        {
            app_url << "/" << i->asString();
        }
        if(mAppQuery.length() > 0)
        {
            app_url << "?" << mAppQuery;
        }
        return app_url.str();
    }
    default:
        LL_WARNS("AppInit") << "Unexpected SLURL type for SLURL string" << (int)mType << LL_ENDL;
        return std::string();
    }
}

std::string LLSLURL::getLoginString() const
{

    std::stringstream unescaped_start;
    switch(mType)
    {
    case LOCATION:
        unescaped_start << "uri:"
                        << mRegion << "&"
                        << ll_round(mPosition[VX]) << "&"
                        << ll_round(mPosition[VY]) << "&"
                        << ll_round(mPosition[VZ]);
        break;
    case HOME_LOCATION:
        unescaped_start << "home";
        break;
    case LAST_LOCATION:
        unescaped_start << "last";
        break;
    default:
        LL_WARNS("AppInit") << "Unexpected SLURL type for login string" << (int)mType << LL_ENDL;
        break;
    }
    return  LLStringFn::xml_encode(unescaped_start.str(), true);
}

bool LLSLURL::operator==(const LLSLURL& rhs)
{
    if(rhs.mType != mType) return false;
    switch(mType)
    {
    case LOCATION:
        return ((mGrid == rhs.mGrid) &&
                (mRegion == rhs.mRegion) &&
                (mPosition == rhs.mPosition));
    case APP:
        return getSLURLString() == rhs.getSLURLString();

    case HOME_LOCATION:
    case LAST_LOCATION:
        return true;
    default:
        return false;
    }
}

bool LLSLURL::operator !=(const LLSLURL& rhs)
{
    return !(*this == rhs);
}

std::string LLSLURL::getLocationString() const
{
    return llformat("%s/%d/%d/%d",
                    mRegion.c_str(),
                    (S32)ll_round(mPosition[VX]),
                    (S32)ll_round(mPosition[VY]),
                    (S32)ll_round(mPosition[VZ]));
}
std::string LLSLURL::asString() const
{
    std::ostringstream result;
    result <<   " mAppCmd:" << getAppCmd() <<
           " mAppPath:" + getAppPath().asString() <<
           " mAppQueryMap:" + getAppQueryMap().asString() <<
           " mAppQuery: " + getAppQuery() <<
           " mGrid: " + getGrid() <<
           " mRegion: " + getRegion() <<
           " mPosition: " <<
           " mType: " << mType <<
           " mPosition: " << mPosition <<
           " mHypergrid: " << mHypergrid;
    return result.str();
}

// <AW: opensim>
std::string LLSLURL::getTypeHumanReadable(SLURL_TYPE type)
{
    switch(type)
    {
    case INVALID:
        return "INVALID";
    case LOCATION:
        return "LOCATION";
    case HOME_LOCATION:
        return "HOME_LOCATION";
    case LAST_LOCATION:
        return "LAST_LOCATION";
    case APP:
        return "APP";
    case HELP:
        return "HELP";
    case EMPTY:
        return "EMPTY";
    default:
        return{};
    }
}
// </AW: opensim>
