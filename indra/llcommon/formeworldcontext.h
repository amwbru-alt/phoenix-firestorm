/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FORME_WORLD_CONTEXT_H
#define FORME_WORLD_CONTEXT_H
#include "formeworldreplica.h"
namespace ForMeWorldProtocol
{
struct Binding
{
    std::string agent, session, region, capability;
    bool operator==(const Binding&) const = default;
    bool valid() const { return uuid(agent) && uuid(session) && uuid(region) && !capability.empty(); }
};
// Explicit invalidation fences A -> B -> A and a failed teleport back to the same capability.
class RequestContext
{
public:
    bool bind(const Binding& value)
    {
        if (value == mBinding) return false;
        mBinding = value; ++mEpoch; return true;
    }
    void invalidate() { mBinding = {}; ++mEpoch; }
    std::uint64_t token() const { return mEpoch; }
    bool matches(std::uint64_t token, const Binding& live) const
    { return token == mEpoch && mBinding.valid() && mBinding == live; }
private:
    Binding mBinding;
    std::uint64_t mEpoch = 0;
};
}
#endif
