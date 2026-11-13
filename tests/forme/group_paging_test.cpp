// Copyright (C) 2026 ForMe.World contributors. SPDX-License-Identifier: LGPL-2.1-or-later
#include "formegrouppaging.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace ForMeGroups;
    assert(boundedPageSize(0) == 1000 && boundedPageSize(50000) == 1000 && boundedPageSize(50) == 50);
    auto page = advancePage(0, 50, 50, true, 123); assert(!page.complete && page.next == 50);
    page = advancePage(page.next, 50, 50, true, 123); assert(!page.complete && page.next == 100);
    page = advancePage(page.next, 50, 23, true, 123); assert(page.complete && page.next == 123);
    assert(advancePage(0, 50, 50, true, 50).complete);
    assert(advancePage(50, 50, 50, false, 200).complete);
    assert(advancePage(50, 50, 0, false, 200).complete);
    assert(advancePage(0xffffffffU, 50, 1, true, 0).complete);
    assert(!advancePage(0, 50, 50, true, 0).complete);
    std::cout << "Group paging policy passed\n";
}
