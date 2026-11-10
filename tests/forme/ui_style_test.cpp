// Copyright (C) 2026 ForMe.World contributors.
// SPDX-License-Identifier: LGPL-2.1-only
#include "formeuistyle.h"
#include <cassert>
#include <string_view>
int main()
{
    using namespace std::literals;
    assert(ForMeUI::iconFor("display") == "Forme_UI_Graphics"sv);
    assert(ForMeUI::iconFor("WORN ITEMS") == "Forme_UI_Hanger"sv);
    assert(ForMeUI::iconFor("input") == "Forme_UI_Cloud"sv);
    assert(ForMeUI::iconFor("im") == "Forme_UI_Shield"sv);
    assert(ForMeUI::iconFor("Delete Selection") == "Forme_UI_Trash"sv);
    assert(ForMeUI::iconFor("panel_profile_notes") == "Forme_UI_Note"sv);
    assert(ForMeUI::iconFor("group_notices_tab_panel") == "Forme_UI_Bell"sv);
    assert(ForMeUI::iconFor("panel_profile_web") == "Forme_UI_Cloud"sv);
    assert(ForMeUI::iconFor("unknown-runtime-tab-id") == "Forme_UI_Window"sv);
}
