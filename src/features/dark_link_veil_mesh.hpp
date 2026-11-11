#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dawnlight::dark_veil {
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 b) const { return {x+b.x, y+b.y, z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x, y-b.y, z-b.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
};
inline float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline float length(Vec3 a) { return std::sqrt(dot(a,a)); }
inline Vec3 normalized(Vec3 a) { const auto n=length(a); return n>0.0001f ? a*(1/n) : Vec3{0,1,0}; }
inline Vec3 limited(Vec3 a, float limit) { const auto n=length(a); return n>limit ? a*(limit/n) : a; }
using Pose = std::array<std::array<float,4>,3>;
inline Vec3 origin(const Pose& p) { return {p[0][3],p[1][3],p[2][3]}; }
inline Vec3 rotate(const Pose& p, Vec3 v) {
    return {p[0][0]*v.x+p[0][1]*v.y+p[0][2]*v.z,
            p[1][0]*v.x+p[1][1]*v.y+p[1][2]*v.z,
            p[2][0]*v.x+p[2][1]*v.y+p[2][2]*v.z};
}
inline bool finite(const Pose& p) {
    for (const auto& row:p) for (float value:row) if (!std::isfinite(value)) return false;
    return true;
}
inline constexpr std::array<unsigned,11> kJoints{1,2,3,7,8,12,13,18,19,23,24};
// Strand roots sit inside the approximate limb volume. Scene depth hides
// their buried base, so the visible ribbon emerges from the model surface.
// The spaces between strands contain no geometry, rather than a faint shell.
inline constexpr std::array<Vec3,11> kSurfaces{{
    {20,9,7},{18,10,7},{10,4,4},
    {28,4,4},{26,3,3},{28,4,4},{26,3,3},
    {35,6,6},{40,4,4},{35,6,6},{40,4,4},
}};
inline constexpr unsigned kStrands=8, kSegments=24;
inline constexpr unsigned kVertexCount=kJoints.size()*kStrands*kSegments*6;
inline constexpr float kRiseSpeed=20.0f;
struct Vertex { Vec3 world, field; float opacity; };

// Features at height h reappear at h + speed*dt after dt: the bends themselves
// travel upward, not just a low-contrast texture on otherwise static geometry.
inline Vec3 curl(float height, float seconds, float seed) {
    const float flow=height-kRiseSpeed*seconds;
    const float phase=seed*2.39996323f;
    return {std::sin(flow*0.18f+phase)+0.28f*std::sin(flow*0.37f+phase*1.7f),0,
            std::cos(flow*0.16f+phase*1.13f)+0.25f*std::sin(flow*0.31f+phase*0.7f)};
}
inline float smooth(float a,float b,float value) {
    const float t=std::clamp((value-a)/(b-a),0.0f,1.0f);
    return t*t*(3-2*t);
}

using StrandOffsets = std::array<Vec3,kJoints.size()*kStrands>;
// Independent, repeatable variation: wide strands need not also be long/dark.
inline float variation(unsigned index, unsigned channel) {
    uint32_t h=(index+1)*0x9e3779b9u+channel*0x85ebca6bu;
    h^=h>>16; h*=0x7feb352du; h^=h>>15; h*=0x846ca68bu; h^=h>>16;
    return float(h & 0xffffu)/65535.0f;
}
struct Life {
    float age, duration;
    unsigned generation;
};
inline Life lifecycle(unsigned i,float seconds) {
    const float duration=3.2f+2.8f*variation(i,6);
    const float clock=seconds/duration+variation(i,7);
    const float generation=std::floor(clock);
    return {clock-generation,duration,static_cast<unsigned>(generation)};
}
inline float life_opacity(float age) {
    return smooth(0,0.14f,age)*(1-smooth(0.62f,1,age));
}
inline Vec3 local_anchor(unsigned part, unsigned strand,float seconds=0) {
    constexpr float pi=3.14159265358979323846f;
    const unsigned i=part*kStrands+strand;
    const auto life=lifecycle(i,seconds);
    const unsigned seed=i+life.generation*kJoints.size()*kStrands;
    const float angle=2*pi*variation(seed,8)+seconds*(0.23f+variation(i,9)*0.22f);
    const auto dim=kSurfaces[part];
    const float slide=0.34f+0.22f*std::sin(seconds*0.7f+variation(seed,10)*2*pi);
    return {dim.x*slide,dim.y*std::cos(angle),dim.z*std::sin(angle)};
}
inline Vec3 anchor(const Pose& pose,unsigned part,unsigned strand,float seconds=0) {
    return origin(pose)+rotate(pose,local_anchor(part,strand,seconds));
}
inline Vec3 outward_at(const Pose& pose,unsigned part,unsigned strand,float seconds) {
    auto local=local_anchor(part,strand,seconds); local.x=0;
    const auto radial=rotate(pose,normalized(local));
    return limited({radial.x,0,radial.z},1);
}

struct Motion {
    Vec3 previousRoot{}, travelLag{};
    StrandOffsets previous{}, localLag{}, lag{}, base{}, velocity{}, outward{};
    std::array<unsigned,kJoints.size()*kStrands> generation{};
    std::array<bool,kJoints.size()*kStrands> released{};
    bool ready=false;
    void reset() { *this={}; }
    void update(const std::array<Pose,kJoints.size()>& poses, Vec3 root, float dt, float seconds) {
        if (!std::isfinite(seconds) || seconds<0) { reset(); return; }
        for (const auto& p:poses) if (!finite(p)) { reset(); return; }
        const bool restart=!ready || !std::isfinite(dt) || dt<=0 || dt>0.25f ||
                           length(root-previousRoot)>120;
        const auto rootDelta=root-previousRoot;
        constexpr float travelResponse=0.10f;
        if (restart) travelLag={};
        else {
            const float blend=-std::expm1(-dt/travelResponse);
            // World displacement per second, independent of sprint provider and
            // render rate. Keep sprint translation outside the small limb cap.
            travelLag=limited(travelLag*(1-blend)-rootDelta*(travelResponse*blend/dt),384.0f);
        }
        for (unsigned part=0;part<poses.size();++part) {
            for (unsigned strand=0;strand<kStrands;++strand) {
                const unsigned i=part*kStrands+strand;
                const auto life=lifecycle(i,seconds);
                const auto current=anchor(poses[part],part,strand,seconds);
                const auto delta=current-previous[i];
                const auto localDelta=delta-rootDelta;
                const float tau=0.12f+0.12f*variation(i,5);
                const bool fresh=restart || generation[i]!=life.generation || length(delta)>100;
                if (fresh) {
                    localLag[i]={}; lag[i]=travelLag*(tau/0.18f);
                    base[i]=current; velocity[i]={}; released[i]=false;
                    outward[i]=outward_at(poses[part],part,strand,seconds);
                } else if (!released[i]) {
                    const float blend=-std::expm1(-dt/tau);
                    // Exact first-order follower for a linearly moving anchor.
                    // Bound limb swings separately; translation remains speed
                    // proportional throughout normal running and sprinting.
                    localLag[i]=limited(localLag[i]*(1-blend)-localDelta*(tau*blend/dt),28.0f);
                    lag[i]=limited(travelLag*(tau/0.18f)+localLag[i],512.0f);
                    base[i]=current;
                    outward[i]=outward_at(poses[part],part,strand,seconds);
                } else {
                    // Detached smoke stays in world space, rising and drifting
                    // with a little inherited momentum instead of chasing Link.
                    base[i]=base[i]+velocity[i]*dt;
                    // Keep the world-space inclination captured on release.
                    // Do not straighten detached smoke or turn it with Link.
                }
                if (!released[i] && life.age>=0.62f) {
                    released[i]=true;
                    const auto bodyMomentum=travelLag*(-0.18f/travelResponse);
                    auto momentum=limited(bodyMomentum+(fresh ? Vec3{}:localDelta*(0.18f/dt)),120.0f);
                    momentum.y=std::max(momentum.y,-8.0f); // retain upward buoyancy while falling
                    velocity[i]=momentum+outward[i]*3+Vec3{0,12,0};
                }
                previous[i]=current; generation[i]=life.generation;
            }
        }
        previousRoot=root; ready=true;
    }
};

inline Vec3 stabilize_ribbon(std::array<Vec3,kSegments+1>& centers,Vec3 eye) {
    const auto base=centers.front();
    const auto view=normalized(eye-base);
    const float distance=std::max(1.0f,length(eye-base));
    const auto span=centers.back()-base;
    const auto projected=span-view*dot(span,view);
    // One transverse axis for the whole ribbon. Rotating each row around its
    // local tangent makes wide bends fold into overlapping triangular fans.
    auto across=cross(projected,view);
    if (length(across)<0.001f) across=cross({0,1,0},view);
    if (length(across)<0.001f) across=cross({1,0,0},view);
    across=normalized(across);
    const auto along=cross(view,across);
    const float travel=std::max(0.0f,dot(span,along));
    for (unsigned r=0;r<=kSegments;++r) {
        const auto offset=centers[r]-base;
        const float depth=std::min(dot(offset,view),distance*0.9f);
        // Retain lateral curls and world depth, but make projected rows advance
        // monotonically. The perspective correction prevents a deep bend from
        // reversing row order. Adjacent quads cannot fold or self-overdraw.
        const float height=travel*(float(r)/kSegments)*(distance-depth)/distance;
        centers[r]=base+across*dot(offset,across)+along*height+view*depth;
    }
    return across;
}

inline void build(const std::array<Pose,kJoints.size()>& poses, const Pose& root,
                  Vec3 eye, const Motion& motion, float seconds, float fade, std::vector<Vertex>& out) {
    out.clear();
    if (!finite(root) || !std::isfinite(seconds) || seconds<0) return;
    for (const auto& pose:poses) if (!finite(pose)) return;
    out.reserve(kVertexCount);
    constexpr float pi=3.14159265358979323846f;
    for (unsigned part=0;part<poses.size();++part) {
        const auto& pose=poses[part];
        for (unsigned strand=0;strand<kStrands;++strand) {
            const unsigned index=part*kStrands+strand;
            const auto life=lifecycle(index,seconds);
            const unsigned variant=index+life.generation*kJoints.size()*kStrands;
            const float seed=float(variant%4096+1);
            const float curlSize=2.0f+variation(variant,0)*4.5f;
            const auto base=motion.ready ? motion.base[index]:anchor(pose,part,strand,seconds);
            const auto outward=motion.ready ? motion.outward[index]:outward_at(pose,part,strand,seconds);
            const auto lag=motion.ready ? motion.lag[index]:Vec3{};
            const float height=(part==2 ? 58.0f : part<2 ? 46.0f : 32.0f)*
                               (0.45f+variation(variant,1)*1.05f);
            // Keep fine wisps while allowing the broadest strands to reach
            // eight times the original 3.4-unit half-width.
            const float widthVariation=variation(variant,2);
            const float halfWidth=0.55f+widthVariation*widthVariation*26.65f;
            const float density=(0.35f+variation(variant,3)*0.65f)*life_opacity(life.age);
            std::array<Vec3,kSegments+1> centers{};
            std::array<std::array<Vertex,2>,kSegments+1> strip{};
            for (unsigned r=0;r<=kSegments;++r) {
                const float t=float(r)/kSegments;
                const float h=height*t;
                const float growth=smooth(0,0.65f,t);
                centers[r]=base+Vec3{0,h,0}+
                    curl(h,seconds,seed)*(curlSize*growth)+
                    outward*(t*t*5)+lag*smooth(0,0.85f,t);
            }
            const auto across=stabilize_ribbon(centers,eye);
            for (unsigned r=0;r<=kSegments;++r) {
                const float t=float(r)/kSegments;
                const float pulse=0.8f+0.2f*std::sin((height*t-kRiseSpeed*seconds)*0.2f+seed);
                const float width=halfWidth*pulse*(0.65f+0.35f*std::sin(pi*t))*std::pow(1-t,0.65f);
                // Attached bases are buried, without a transparent body gap.
                // Once released, gradually feather the free lower end as well.
                const float baseFade=1-smooth(0.62f,0.72f,life.age)*(1-smooth(0,0.12f,t));
                const float opacity=fade*density*baseFade*(1-smooth(0.6f,1,t));
                const float flow=(height*t-kRiseSpeed*seconds)*0.13f;
                for (unsigned side=0;side<2;++side) {
                    const float u=side ? 1.0f:-1.0f;
                    strip[r][side]={centers[r]+across*(u*width),{u,flow,seed},opacity};
                }
            }
            // Each strand is an independent strip. Never connect neighbouring
            // roots or limbs into a translucent enclosing surface.
            for (unsigned r=0;r<kSegments;++r)
                out.insert(out.end(),{strip[r][0],strip[r][1],strip[r+1][0],
                                      strip[r][1],strip[r+1][1],strip[r+1][0]});
        }
    }
}
}
