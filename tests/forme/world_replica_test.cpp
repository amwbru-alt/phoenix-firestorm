// Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later.
#include "formeworldreplica.h"
#include <cassert>
#include <iostream>
using namespace ForMeWorldProtocol;
int main()
{
    std::uint64_t revisionValue=0;
    assert(revision("9007199254740993",revisionValue)&&revisionValue==9007199254740993ULL);
    assert(revision("9223372036854775807",revisionValue));
    for(const std::string s:{"-1","1.1","01","+2","9223372036854775808","18446744073709551615","1evil",""})assert(!revision(s,revisionValue));
    const std::string region="11111111-2222-4333-8444-555555555555", epoch="11111111-2222-4333-8444-555555555556", id="11111111-2222-4333-8444-555555555557";
    assert(uuid(region)&&!uuid("00000000-0000-0000-0000-000000000000"));
    Replica<std::string> cache;
    Page<std::string> first{region,epoch,1,true,{{id,false,{id,1,1,"chair"}}}};
    assert(cache.apply(first,region)&&cache.entities().at(id).data=="chair");
    Page<std::string> update{region,epoch,2,false,{{id,false,{id,2,1,"sofa"}}}};
    assert(cache.apply(update,region));assert(cache.apply(update,region));
    assert(!cache.apply(first,region)); // A delayed initial reply cannot rewind a newer snapshot.
    auto gap=update;gap.cursor=4;assert(!cache.apply(gap,region));
    auto invalid=update;invalid.cursor=3;invalid.changes[0].entity.revision=1;
    assert(!cache.apply(invalid,region)&&cache.cursor()==2&&cache.entities().at(id).data=="sofa");
    invalid=update;invalid.cursor=3;invalid.changes.push_back(invalid.changes[0]);assert(!cache.apply(invalid,region));
    invalid=update;invalid.cursor=3;invalid.region="11111111-2222-4333-8444-555555555558";assert(!cache.apply(invalid,region));
    invalid=update;invalid.cursor=3;invalid.epoch="11111111-2222-4333-8444-555555555558";assert(!cache.apply(invalid,region));
    Page<std::string> removal{region,epoch,3,false,{{id,true,{}}}};
    assert(cache.apply(removal,region)&&cache.entities().empty());
    auto replacement=first;replacement.epoch="11111111-2222-4333-8444-555555555559";
    assert(cache.apply(replacement,region)&&cache.cursor()==1);
    auto oversized=replacement;oversized.epoch="11111111-2222-4333-8444-555555555550";oversized.changes.resize(513);
    assert(!cache.apply(oversized,region)&&cache.entities().at(id).data=="chair");
    cache.clear();assert(cache.entities().empty()&&cache.cursor()==0&&cache.epoch().empty());assert(!cache.apply(update,region));
    std::cout<<"PASS bounded world replica: precision, retries, reordering, revocation/removal, atomic rejection and region/session reset\n";
}
