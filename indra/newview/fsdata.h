/**
 * @file fsdata.h
 * @brief Downloadable dymatic xml data for viewer features.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2011-2013 Techwolf Lupindo
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
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#ifndef FS_DATA_H
#define FS_DATA_H

#include "llinstantmessage.h"
#include "llsingleton.h"
#include "llavatarnamecache.h"

class FSData : public LLSingleton<FSData>
{
    LOG_CLASS(FSData);

    LLSINGLETON(FSData);
    virtual ~FSData();

public:
    void startDownload();
    void downloadAgents();
    void addAgents();

    LLSD resolveClientTag(const LLUUID& id, bool new_system, const LLColor4& new_system_color) const;

    enum flags_t
    {
        SUPPORT     = (1 << 0), //0x01 1
        DEVELOPER   = (1 << 1), //0x02 2
        QA          = (1 << 2), //0x04 4
        CHAT_COLOR  = (1 << 3), //0x08 8
        NO_SUPPORT  = (1 << 4), //0x10 16
        NO_USE      = (1 << 5), //0x20 32
        NO_SPAM     = (1 << 6), //0x40 64
        GATEWAY     = (1 << 7), //0x80 128
    };

    bool isDeveloper(const LLUUID& avatar_id) const;
    bool isSupport(const LLUUID& avatar_id) const;
    bool isQA(const LLUUID& avatar_id) const;
    bool isFirestormGroup(const LLUUID& id) const;
    bool isSupportGroup(const LLUUID& id) const;
    bool isTestingGroup(const LLUUID& id) const;

    // returns -1 if agent is not found.
    S32 getAgentFlags(const LLUUID& avatar_id) const;

    LLSD allowedLogin() const;

    bool enableLegacySearch() const { return true; }

    std::string processRequestForInfo(const LLUUID& requester, const std::string& message, const std::string& name, const LLUUID& sessionid);
    static LLSD getSystemInfo();

    std::string getOpenSimMOTD() const { return mOpenSimMOTD; }
    void selectNextMOTD();

    bool getFSDataDone() const { return true; }
    bool getAgentsDone() const { return true; }

    bool isAgentFlag(const LLUUID& agent_id, FSData::flags_t flag) const;

    const std::string& getLatestVersion() const { return mLatestVersion; }
    const std::string& getDownloadURL() const { return mDownloadURL; }

private:
    bool mRequested = false;
    std::string mOpenSimMOTD;
    std::string mLatestVersion;
    std::string mDownloadURL;

};

#endif // FS_DATA_H
