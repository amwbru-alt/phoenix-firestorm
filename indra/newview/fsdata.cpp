/**
 * @file fsdata.cpp
 * @brief Downloadable dymatic xml data for viewer features.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2011-2013 Techwolf Lupindo
 * Portions Copyright (C)
 *   2011 Wolfspirit Magic
 *   2012 Ansariel Hiller @ Second Life
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

#include "llviewerprecompiledheaders.h"
#include "fsdata.h"
#include "formeviewerdata.h"
#include "llappviewer.h"
#include "llagent.h"
#include "llcorehttputil.h"
#include "llcoros.h"
#include "llversioninfo.h"

FSData::FSData() = default;
FSData::~FSData() = default;

void FSData::startDownload()
{
    if (mRequested) return;
    mRequested = true;
    // Public metadata never delays login, alters permissions or imports a cache
    // created by a different viewer. Errors leave the normal login MOTD intact.
    LLCoros::instance().launch("ForMeViewerData", [this]() {
        auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeViewerData", LLCore::HttpRequest::DEFAULT_POLICY_ID);
        auto request = std::make_shared<LLCore::HttpRequest>();
        auto options = std::make_shared<LLCore::HttpOptions>();
        options->setFollowRedirects(false);
        options->setTimeout(5);
        options->setRetries(0);
        LLSD result;
        try { result = adapter->getJsonAndSuspend(request, ForMeViewerData::Endpoint, options); }
        catch (...) { return; }
        const auto status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
        if (!status || !result.isMap() || !result["version"].isInteger() || !result["service"].isString()
            || !result["message_of_the_day"].isString() || !result["latest_version"].isString() || !result["download_url"].isString()) return;
        const auto motd=result["message_of_the_day"].asString(), latest=result["latest_version"].asString(), url=result["download_url"].asString();
        if (!ForMeViewerData::valid(result["version"].asInteger(),result["service"].asString(),motd,latest,url)) return;
        mOpenSimMOTD=motd; mLatestVersion=latest; mDownloadURL=url;
        if (!motd.empty()) gAgent.mMOTD=motd;
    });
}

// Compatibility entry points for existing UI. Staff roles, colors and mute
// decisions come from the grid, never from a downloaded vendor roster.
void FSData::downloadAgents() {}
void FSData::addAgents() {}
void FSData::selectNextMOTD() {}
LLSD FSData::resolveClientTag(const LLUUID&, bool, const LLColor4&) const { return LLSD(); }
S32 FSData::getAgentFlags(const LLUUID&) const { return -1; }
bool FSData::isSupport(const LLUUID&) const { return false; }
bool FSData::isDeveloper(const LLUUID&) const { return false; }
bool FSData::isQA(const LLUUID&) const { return false; }
bool FSData::isFirestormGroup(const LLUUID&) const { return false; }
bool FSData::isSupportGroup(const LLUUID&) const { return false; }
bool FSData::isTestingGroup(const LLUUID&) const { return false; }
bool FSData::isAgentFlag(const LLUUID&, flags_t) const { return false; }
LLSD FSData::allowedLogin() const { return LLSD(); }
std::string FSData::processRequestForInfo(const LLUUID&, const std::string& message, const std::string&, const LLUUID&) { return message; }

LLSD FSData::getSystemInfo()
{
    LLSD info = LLAppViewer::instance()->getViewerInfo();

    std::string sysinfo1("\n");
    sysinfo1 += llformat("%s %s (%d) %s %s (%s %dbit / %s) %s\n\n", LLAppViewer::instance()->getSecondLifeTitle().c_str(),
                                                                    LLVersionInfo::getInstance()->getShortVersion().c_str(),
                                                                    LLVersionInfo::getInstance()->getBuild(),
                                                                    info["BUILD_DATE"].asString().c_str(), info["BUILD_TIME"].asString().c_str(),
                                                                    LLVersionInfo::getInstance()->getChannel().c_str(),
                                                                    info["ADDRESS_SIZE"].asInteger(),
                                                                    info["SIMD"].asString().c_str(),
                                                                    info["BUILD_TYPE"].asString().c_str());
    sysinfo1 += llformat("Build with %s version %s\n\n", info["COMPILER"].asString().c_str(), info["COMPILER_VERSION"].asString().c_str());
    sysinfo1 += llformat("Location: %s (%s)\n", info["REGION"].asString().c_str(), info["HOSTNAME"].asString().c_str(), info["HOSTIP"].asString().c_str());
    sysinfo1 += llformat("%s\n\n", info["SERVER_VERSION"].asString().c_str());

    sysinfo1 += llformat("CPU: %s\n", info["CPU"].asString().c_str());
    sysinfo1 += llformat("Memory: %d MB (Used: %d MB)\n", info["MEMORY_MB"].asInteger(), info["USED_RAM"].asInteger());
    sysinfo1 += llformat("OS: %s\n", info["OS_VERSION"].asString().c_str());
    sysinfo1 += llformat("Graphics Card Vendor: %s\n", info["GRAPHICS_CARD_VENDOR"].asString().c_str());
    sysinfo1 += llformat("Graphics Card: %s\n", info["GRAPHICS_CARD"].asString().c_str());
    sysinfo1 += llformat("VRAM: %d MB\n", info["GRAPHICS_CARD_MEMORY"].asInteger());
    sysinfo1 += llformat("VRAM (Detected): %d MB\n", info["GRAPHICS_CARD_MEMORY_DETECTED"].asInteger());
    sysinfo1 += llformat("VRAM (Budget): %s\n", info["VRAM_BUDGET_ENGLISH"].asString().c_str());

    if (info.has("GRAPHICS_DRIVER_VERSION"))
    {
        sysinfo1 += llformat("Graphics Card Driver Version: %s\n", info["GRAPHICS_DRIVER_VERSION"].asString().c_str());
    }

    std::string sysinfo2("\n");
    sysinfo2 += llformat("OpenGL Version: %s\n\n", info["OPENGL_VERSION"].asString().c_str());
    sysinfo2 += llformat("libcurl Version: %s\n", info["LIBCURL_VERSION"].asString().c_str());
    sysinfo2 += llformat("J2C Decoder Version: %s\n", info["J2C_VERSION"].asString().c_str());
    sysinfo2 += llformat("Audio Driver Version: %s\n", info["AUDIO_DRIVER_VERSION"].asString().c_str());
    sysinfo2 += llformat("%s\n", info["LIBCEF_VERSION"].asString().c_str());
    sysinfo2 += llformat("LibVLC Version: %s\n", info["LIBVLC_VERSION"].asString().c_str());

    sysinfo2 += llformat("Vivox Version: %s\n", info["VOICE_VERSION"].asString().c_str());
    sysinfo2 += llformat("Packets Lost: %.0f/%.0f (%.1f%%)\n\n", info["PACKETS_LOST"].asReal(), info["PACKETS_IN"].asReal(), info["PACKETS_PCT"].asReal());

    sysinfo2 += llformat("RLVa: %s\n", info["RLV_VERSION"].asString().c_str());
    sysinfo2 += llformat("Mode: %s\n", info["MODE"].asString().c_str());
    sysinfo2 += llformat("Skin: %s (%s)\n", info["SKIN"].asString().c_str(), info["THEME"].asString().c_str());
    sysinfo2 += llformat("Window Size: %sx%s px\n", info["WINDOW_WIDTH"].asString().c_str(), info["WINDOW_HEIGHT"].asString().c_str());
#if LL_DARWIN
    sysinfo2 += llformat("HiDPI: %s\n", info["HIDPI"].asBoolean() ? "Enabled" : "Disabled");
#endif
    sysinfo2 += llformat("Font: %s\n", info["FONT"].asString().c_str());
    sysinfo2 += llformat("Font Size Adjustment: %d pt\n", info["FONT_SIZE"].asInteger());
    sysinfo2 += llformat("Font Screen DPI: %d\n", info["FONT_SCREEN_DPI"].asInteger());
    sysinfo2 += llformat("UI Scaling: %.3f\n", info["UI_SCALE_FACTOR"].asReal());
    sysinfo2 += llformat("Draw Distance: %d m\n", info["DRAW_DISTANCE"].asInteger());
    sysinfo2 += llformat("Bandwidth: %d kbit/s\n", info["BANDWIDTH"].asInteger());
    sysinfo2 += llformat("LOD Factor: %.3f\n", info["LOD"].asReal());
    sysinfo2 += llformat("Render quality: %s\n", info["RENDERQUALITY_FSDATA_ENGLISH"].asString().c_str());
    sysinfo2 += "Disk cache: " + info["DISK_CACHE_INFO"].asString();

    LLSD sysinfos;
    sysinfos["Part1"] = sysinfo1;
    sysinfos["Part2"] = sysinfo2;

    return sysinfos;
}
