// Copyright (C) 2026 ForMe.World contributors. SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef FORME_GROUP_PAGING_H
#define FORME_GROUP_PAGING_H
#include <cstdint>
#include <limits>
namespace ForMeGroups
{
    inline std::uint32_t boundedPageSize(std::uint32_t requested)
    {
        return requested == 0 || requested > 1000 ? 1000 : requested;
    }
    struct Page { std::uint32_t next; bool complete; };
    inline Page advancePage(std::uint32_t start, std::uint32_t size, std::uint32_t received, bool progress, std::uint32_t total)
    {
        if (received > std::numeric_limits<std::uint32_t>::max() - start) return {start, true};
        const auto next = start + received;
        return {next, !progress || received == 0 || received < size || (total > 0 && next >= total)};
    }
}
#endif
