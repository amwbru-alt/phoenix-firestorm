// Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later.
#include "formeworldphysics.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace ForMeWorldPhysics;
int main()
{
    Profile valid{"dynamic","convex",700,.6,.1,.5};assert(valid.valid());
    for(auto mode:{"cloth","hair","fluid","kinematic"}){auto p=valid;p.mode=mode;assert(!p.valid());}
    for(auto shape:{"sdf","unknown","none"}){auto p=valid;p.shape=shape;assert(!p.valid());}
    for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-1.0,22588.0}){auto p=valid;p.density=value;assert(!p.valid());}
    auto p=valid;p.gravity=-1;assert(p.valid());p.gravity=28;assert(p.valid());p.gravity=28.01;assert(!p.valid());
    p=valid;p.restitution=1.001;assert(!p.valid());p=valid;p.friction=256;assert(!p.valid());
    assert(!preset("unknown")&&!preset("custom"));
    for(const auto& item:Presets){assert(Profile({"static","primitive",item.density,item.friction,item.restitution,1}).valid());}
    assert(preset("rubber")->restitution>preset("stone")->restitution&&preset("ice")->friction<preset("wood")->friction);
    std::cout<<"PASS physical profile bounds, unsupported solver rejection and material presets\n";
}
