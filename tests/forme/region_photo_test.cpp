// Copyright (C) 2026, ForMe.World contributors.
// SPDX-License-Identifier: LGPL-2.1-only
#include "formeregionphoto.h"
#include <cassert>
#include <iostream>
int main()
{
    const std::string photo = "/web/assets/img/uploads/marketplace-region-photo.jpg";
    const std::string expected = std::string(ForMeGrid::LoginURI) + photo.substr(1);
    assert(ForMeRegionPhoto::url(photo) == expected);
    assert(ForMeRegionPhoto::url(expected) == expected);
    for (const char* missing : {"", "/map/map-1-1000-1000-objects.jpg", "/map-1-1000-1000-objects.jpg",
        "http://185.173.93.200:8002/map/map-1-1000-1000-objects.jpg", "https://outside.example/photo.png",
        "//outside.example/photo.png", "/web/assets/img/../private.jpg", "/web/assets/img/%2e%2e/private.jpg"})
        assert(ForMeRegionPhoto::url(missing).empty());
    std::cout << "PASS published region photo URL, same origin and no terrain tile substitution\n";
}
