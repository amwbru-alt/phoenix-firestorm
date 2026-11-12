/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "formemarketpolicy.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace ForMeMarket;
    for (int status : {0, 200, 408, 409, 422, 429, 500, 502, 503, 504})
        assert(!releaseReceipt(false, status, ""));
    for (int status : {400, 401, 402, 403, 404}) assert(releaseReceipt(false, status, ""));
    assert(releaseReceipt(true, 200, ""));
    assert(releaseReceipt(false, 409, "quote_changed"));
    assert(releaseReceipt(false, 409, "stock_unavailable"));
    assert(!releaseReceipt(false, 409, "partial_delivery"));
    assert(!releaseReceipt(false, 500, "refund_unknown"));
    const std::string base = "http://185.173.93.200:8002/";
    assert(photoURL("/web/assets/product.jpg", base) == base + "web/assets/product.jpg");
    assert(photoURL(base + "web/assets/product.png", base) == base + "web/assets/product.png");
    for (const auto* value : {"file:///etc/passwd", "data:image/png;base64,x", "https://example.com/photo.png",
        "//example.com/web/assets/x", "/web/assets/../secret", "/web/assets/%2e%2e/secret",
        "/web/assets/x?next=foreign", "/web/assets/x#fragment", "/web/assets/x\\..\\secret",
        "http://185.173.93.200:8002.evil/web/assets/x", "/web/assets/x\r\nHost: evil"})
        assert(photoURL(value, base).empty());
    std::cout << "PASS uncertain payment recovery and trusted product photo URLs\n";
}
