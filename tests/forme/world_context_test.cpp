// Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later.
#include "formeworldcontext.h"
#include <cassert>
#include <iostream>
using namespace ForMeWorldProtocol;
int main()
{
    Binding a{"11111111-1111-1111-1111-111111111111","22222222-2222-2222-2222-222222222222",
        "33333333-3333-3333-3333-333333333333","https://fixture.invalid/cap/a"};
    auto b=a;b.region="44444444-4444-4444-4444-444444444444";b.capability="https://fixture.invalid/cap/b";
    RequestContext context;
    assert(!context.matches(context.token(),a));assert(context.bind(a));auto first=context.token();
    assert(context.matches(first,a));assert(!context.bind(a));assert(context.matches(first,a));
    assert(!context.matches(first,b)); // Live root changed before the next polling tick.
    context.invalidate();context.invalidate();context.bind(a);auto returned=context.token();
    assert(!context.matches(first,a)&&context.matches(returned,a)); // A -> B -> A without an idle poll in B.
    context.invalidate();context.bind(a);auto retry=context.token();
    assert(!context.matches(returned,a)&&context.matches(retry,a)); // Failed teleport back to A.
    auto login=a;login.session="55555555-5555-5555-5555-555555555555";
    assert(!context.matches(retry,login));context.bind(login);assert(!context.matches(retry,a));
    auto rotated=login;rotated.capability="https://fixture.invalid/cap/a2";
    auto oldCap=context.token();context.bind(rotated);assert(!context.matches(oldCap,rotated));
    auto missing=rotated;missing.capability.clear();context.bind(missing);assert(!context.matches(context.token(),missing));
    auto invalid=rotated;invalid.region="00000000-0000-0000-0000-000000000000";context.bind(invalid);assert(!context.matches(context.token(),invalid));
    context.bind(b);auto request=context.token();context.invalidate();context.bind(b);
    Replica<int> projection;
    Page<int> oldPage{b.region,"66666666-6666-6666-6666-666666666666",1,true,{{a.agent,false,{a.agent,1,1,9}}}};
    if(context.matches(request,b))projection.apply(oldPage,b.region);
    assert(projection.entities().empty()&&projection.cursor()==0);
    assert(context.matches(context.token(),b)&&projection.apply(oldPage,b.region)&&projection.entities().size()==1);
    std::cout<<"PASS root/session/capability bindings, unobserved A-B-A, failed teleport and stale projection responses\n";
}
