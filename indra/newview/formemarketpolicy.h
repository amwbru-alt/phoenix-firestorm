/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include <string>
#include <string_view>

namespace ForMeMarket
{
// A lost response, stock reservation conflict, partial delivery or failed refund
// retains the original receipt. Retrying uses that same server journal entry.
inline bool releaseReceipt(bool success, int status, std::string_view code)
{
    return success || code == "quote_changed" || code == "stock_unavailable"
        || status == 400 || status == 401 || status == 402 || status == 403 || status == 404;
}

inline std::string photoURL(std::string value, const std::string& base)
{
    if (value.starts_with("/web/assets/")) value = base + value.substr(1);
    if (!value.starts_with(base + "web/assets/") || value.find("..") != std::string::npos
        || value.find_first_of("%\\\r\n\t?#") != std::string::npos) return {};
    return value;
}
}
