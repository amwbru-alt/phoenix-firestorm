/**
 * @file formeuistyle.h
 * @brief Stable icon roles for native ForMe window and tab identifiers.
 * Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#ifndef LL_FORMEUISTYLE_H
#define LL_FORMEUISTYLE_H
#include <string>
#include <string_view>
namespace ForMeUI
{
inline const char* iconFor(std::string_view identifier)
{
    std::string name(identifier);
    for (char& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    const auto has = [&name](std::string_view word) { return name.find(word) != std::string::npos; };
    if (has("trash") || has("delete") || has("remove")) return "Forme_UI_Trash";
    if (has("copy")) return "Forme_UI_Copy";
    if (has("paste")) return "Forme_UI_Note";
    if (has("rename") || has("edit")) return "Forme_UI_Edit";
    if (has("save")) return "Forme_UI_Save";
    if (has("upload") || has("import")) return "Forme_UI_Upload";
    if (has("export") || has("download")) return "Forme_UI_Download";
    if (has("recent") || has("history") || has("log")) return "Forme_UI_Clock";
    if (has("favorite") || has("pick")) return "Forme_UI_Star";
    if (has("worn") || has("outfit") || has("appearance")) return "Forme_UI_Hanger";
    if (has("inventory") || has("all items") || has("market")) return "Forme_UI_Inventory";
    if (has("folder")) return "Forme_UI_Folder";
    if (has("note")) return "Forme_UI_Note";
    if (has("web")) return "Forme_UI_Cloud";
    if (has("notice") || has("classified")) return "Forme_UI_Bell";
    if (has("land") || has("estate")) return "Forme_UI_Map";
    if (has("money")) return "Forme_UI_Database";
    if (has("group") || has("friend") || has("people") || has("member") || has("participant")) return "Forme_UI_People";
    if (has("profile") || has("avatar") || has("secondlife") || has("firstlife")) return "Forme_UI_Profile";
    if (name == "im") return "Forme_UI_Shield";
    if (has("firestorm")) return "Forme_UI_Star";
    if (has("privacy") || has("access") || has("permission") || has("block") || has("rlv") || has("security")) return "Forme_UI_Shield";
    if (has("chat") || has("communic") || has("conversation") || has("im_panel")) return "Forme_UI_Chat";
    if (has("display") || has("graphic") || has("texture") || has("image")) return "Forme_UI_Graphics";
    if (has("audio") || has("sound") || has("media")) return "Forme_UI_Sound";
    if (has("network") || has("input") || has("setup") || has("web")) return "Forme_UI_Cloud";
    if (has("camera") || has("snapshot")) return "Forme_UI_Camera";
    if (has("move") || has("motion")) return "Forme_UI_Move";
    if (has("control")) return "Forme_UI_Controls";
    if (has("notif") || has("notice") || has("alert")) return "Forme_UI_Bell";
    if (has("skin") || has("theme")) return "Forme_UI_Brush";
    if (has("crash") || has("debug")) return "Forme_UI_Bug";
    if (has("backup") || has("cache")) return "Forme_UI_Database";
    if (has("script")) return "Forme_UI_Code";
    if (has("note")) return "Forme_UI_Note";
    if (has("search")) return "Forme_UI_Search";
    if (has("land") || has("region") || has("estate") || has("place") || has("teleport")) return "Forme_UI_Map";
    if (has("build") || has("object") || has("content") || has("creation")) return "Forme_UI_Cube";
    if (has("advanc") || has("performance") || has("font")) return "Forme_UI_Sliders";
    if (has("pref") || has("setting") || has("general")) return "Forme_UI_Settings";
    if (has("help") || has("about") || has("propert")) return "Forme_UI_Info";
    return "Forme_UI_Window";
}
}
#endif
