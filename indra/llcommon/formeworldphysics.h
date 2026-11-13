/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#ifndef FORME_WORLD_PHYSICS_H
#define FORME_WORLD_PHYSICS_H
#include <array>
#include <cmath>
#include <string_view>
namespace ForMeWorldPhysics
{
struct Profile
{
    std::string_view mode, shape;
    double density=1000,friction=.6,restitution=.1,gravity=1;
    bool phantom=false;
    bool valid() const
    {
        return (mode=="static"||mode=="dynamic")&&(shape=="primitive"||shape=="convex")
            &&std::isfinite(density)&&density>=1&&density<=22587&&std::isfinite(friction)&&friction>=0&&friction<=255
            &&std::isfinite(restitution)&&restitution>=0&&restitution<=1&&std::isfinite(gravity)&&gravity>=-1&&gravity<=28;
    }
};
struct Preset { std::string_view id;double density,friction,restitution; };
inline constexpr std::array<Preset,5> Presets={{{"wood",700,.6,.1},{"metal",7850,.3,.05},{"rubber",1100,.9,.8},{"stone",2600,.8,.05},{"ice",920,.02,.05}}};
inline const Preset* preset(std::string_view id)
{for(const auto& item:Presets)if(item.id==id)return &item;return nullptr;}
}
#endif
