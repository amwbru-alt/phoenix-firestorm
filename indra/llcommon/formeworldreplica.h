/** ForMe.World entity replication. Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FORME_WORLD_REPLICA_H
#define FORME_WORLD_REPLICA_H
#include <charconv>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ForMeWorldProtocol
{
inline bool revision(const std::string& text, std::uint64_t& value)
{
    if (text.empty() || text.size() > 19 || (text.size() > 1 && text[0] == '0')) return false;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc() && result.ptr == text.data() + text.size() && value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
}
inline bool uuid(const std::string& text)
{
    if (text.size() != 36 || text == "00000000-0000-0000-0000-000000000000") return false;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (text[i] != '-') return false; }
        else if (!((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f'))) return false;
    }
    return true;
}
template<class Payload> struct Entity
{
    std::string id;
    std::uint64_t revision = 0, authority = 0;
    Payload data;
};
template<class Payload> struct Change { std::string id; bool deleted; Entity<Payload> entity; };
template<class Payload> struct Page
{
    std::string region, epoch;
    std::uint64_t cursor = 0;
    bool reset = false;
    std::vector<Change<Payload>> changes;
};
template<class Payload> class Replica
{
public:
    void clear() { mRegion.clear(); mEpoch.clear(); mCursor = 0; mEntities.clear(); }
    const std::string& epoch() const { return mEpoch; }
    std::uint64_t cursor() const { return mCursor; }
    const std::map<std::string, Entity<Payload>>& entities() const { return mEntities; }
    bool apply(const Page<Payload>& page, const std::string& expectedRegion)
    {
        if (!uuid(page.region) || !uuid(page.epoch) || page.region != expectedRegion || page.cursor == 0 || page.changes.size() > 512) return false;
        if (!page.reset && page.cursor != mCursor && page.cursor != mCursor + 1) return false;
        if (!page.reset && (page.region != mRegion || page.epoch != mEpoch || page.cursor < mCursor || mEpoch.empty())) return false;
        if (page.region == mRegion && page.epoch == mEpoch && page.cursor == mCursor) return true;
        if (page.region == mRegion && page.epoch == mEpoch && page.cursor < mCursor) return false;
        auto next = page.reset ? std::map<std::string, Entity<Payload>>() : mEntities;
        std::set<std::string> seen;
        for (const auto& change : page.changes)
        {
            if (!uuid(change.id) || !seen.insert(change.id).second) return false;
            if (change.deleted) { if (page.reset) return false; next.erase(change.id); continue; }
            const auto& entity = change.entity;
            if (entity.id != change.id || entity.revision == 0 || entity.authority == 0) return false;
            auto previous = next.find(change.id);
            if (previous != next.end() && (entity.revision < previous->second.revision || entity.authority < previous->second.authority)) return false;
            next[change.id] = entity;
        }
        if (next.size() > 256) return false;
        mEntities = std::move(next); mRegion = page.region; mEpoch = page.epoch; mCursor = page.cursor; return true;
    }
private:
    std::string mRegion, mEpoch;
    std::uint64_t mCursor = 0;
    std::map<std::string, Entity<Payload>> mEntities;
};
}
#endif
