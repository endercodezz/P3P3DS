#include "vcs_project2dfx.hpp"
#include "vcs_project2dfx_lights.hpp"
#include "ge_gpu_backend.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vcs {
void install_draw_distance_patch(psprecomp::Runtime &, const std::filesystem::path &);
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr std::uint32_t kGpGameTimer = 0x1D64u;
constexpr std::uint32_t kGpClockHours = 0x1DE0u;
constexpr std::uint32_t kGpClockMinutes = 0x1DE1u;

constexpr std::uint32_t kHeliHeight1 = 0x08B01D50u;
constexpr std::uint32_t kHeliHeight2 = 0x08B01DA0u;

// ULUS-10160 camera object used by the upstream Project2DFX integration. The original
// Project2DFX resolves this address from the game's code and reads pCamPos at
// +0x9B0.  CSprite::CalcScreenCoors uses the affine world-to-camera matrix at
// +0xA60 in this exact EBOOT (the generated function at 0x08AA82D4 loads it).
// Keeping these together makes the native port use the same camera as the
// original mod instead of guessing it from whichever GE draw happened last.
constexpr std::uint32_t kTheCamera = 0x08BC7E30u;
constexpr std::uint32_t kCameraPosition = kTheCamera + 0x09B0u;
constexpr std::uint32_t kCalcScreenCoors = 0x08AA82D4u;
constexpr std::uint32_t kScreenDepth = 0x08AA8278u;
constexpr std::uint32_t kIsolatedReturn = 0xFFFFFFFFu;
constexpr std::uint32_t kProjectionScratchSize = 0x1000u;

struct Config {
    bool enabled{};
    bool lod_lights{true};
    bool traffic_lights{true};
    bool blinking_lights{true};
    bool sky_gfx{true};
    bool heli_height{true};
    bool distance_growth{true};
    std::uint32_t corona_limit{};
    std::uint32_t corona_segments{12u};
    float corona_radius_multiplier{0.5f};
    float corona_far_clip{500.0f};
    float corona_intensity{1.0f};
    float heli_max_height{800.0f};
    std::uint32_t stars_per_side{100u};
    float smallest_star{0.15f};
    float middle_star{0.60f};
    float biggest_star{1.20f};
    float biggest_star_chance{20.0f};
    std::uint32_t star_seed{0x2DF0A160u};
    bool performance_log{};
    std::uint32_t performance_log_interval{120u};
};

struct Vec3 { float x{}, y{}, z{}; };

struct CameraSnapshot {
    std::array<float, 12> view{};
    std::array<float, 16> projection{};
    // Guest CSprite projection state captured while this exact GE world-camera
    // variant is being drawn. Reading TheCamera later at VBlank can observe the
    // next simulation/camera state and makes fixed lights slide while turning.
    std::array<float, 12> sprite_view{};
    Vec3 camera_position{};
    float scale_x{};
    float scale_y{};
    float scale_z{};
    float center_x{};
    float center_y{};
    float center_z{};
    float offset_x{};
    float offset_y{};
    float clip_x_scale{1.0f};
    GeGpuDrawDescriptor target{};
    std::array<std::uint64_t,8> depth_function_weights{};
    std::uint64_t weight{};
    bool sprite_camera_valid{};
    bool valid{};
};

struct Star { float x{}, y{}, size{}; };

struct FrameDiagnostics {
    std::uint64_t lights_scanned{};
    std::uint64_t lights_in_range{};
    std::uint64_t lights_projected{};
    std::uint64_t lights_emitted{};
    std::uint64_t light_vertices{};
    std::uint64_t stars_projected{};
    std::uint64_t star_vertices{};
};

struct PerformanceAggregate {
    std::uint64_t frames{};
    std::uint64_t cpu_nanoseconds{};
    FrameDiagnostics totals{};
};

Config g_cfg{};
std::filesystem::path g_ini_path;
psprecomp::Runtime *g_runtime{};
std::uint32_t g_projection_scratch{};
// A render target can contain a main world pass plus mirrors, reflections and
// other projected passes with different cameras.  Keep those variants apart;
// collapsing them into one entry and overwriting it on every draw made the
// final special pass become Project2DFX's camera, so fixed map lights appeared
// to move with the player.
std::unordered_map<std::uint32_t, std::vector<CameraSnapshot>> g_camera_candidates;
// Stage 43 hot-camera shortcut. The GE camera normally stays unchanged across
// long runs of object draws; a monotonic camera revision lets those draws avoid
// finite scans, unordered_map lookup and full matrix comparisons entirely.
CameraSnapshot *g_hot_camera{};
std::uint64_t g_hot_camera_revision{};
std::uint32_t g_hot_camera_target{};
std::array<std::vector<Star>, 5> g_stars;
std::uint64_t g_last_rendered_vblank = std::numeric_limits<std::uint64_t>::max();
FrameDiagnostics g_frame_diagnostics{};
PerformanceAggregate g_performance{};
std::ofstream g_performance_log;
GeGpuBackendReport g_previous_backend_report{};

std::string trim(std::string s) {
    auto ws=[](unsigned char c){return std::isspace(c)!=0;};
    while(!s.empty() && ws(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while(!s.empty() && ws(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}
std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});
    return s;
}
std::string uncomment(std::string s) {
    bool quote=false; char q=0;
    for(std::size_t i=0;i<s.size();++i){
        if((s[i]=='\''||s[i]=='\"')){if(!quote){quote=true;q=s[i];}else if(q==s[i])quote=false;}
        else if(!quote && (s[i]==';'||s[i]=='#')){s.resize(i);break;}
    }
    return trim(std::move(s));
}
bool to_bool(std::string s,bool &v){s=lower(trim(std::move(s)));if(s=="1"||s=="true"||s=="yes"||s=="on"){v=true;return true;}if(s=="0"||s=="false"||s=="no"||s=="off"){v=false;return true;}return false;}
bool to_float(std::string s,float &v){s=trim(std::move(s));char*e=nullptr;float x=std::strtof(s.c_str(),&e);if(e==s.c_str()||*e||!std::isfinite(x))return false;v=x;return true;}
bool to_u32(std::string s,std::uint32_t &v){s=trim(std::move(s));char*e=nullptr;unsigned long x=std::strtoul(s.c_str(),&e,0);if(e==s.c_str()||*e)return false;v=std::uint32_t(std::min<unsigned long>(x,0xFFFFFFFFul));return true;}

Config read_config(const std::filesystem::path &path) {
    Config c{};
    std::ifstream f(path);
    if(!f) return c;
    std::string sec,line;
    bool saw_any=false;
    while(std::getline(f,line)){
        line=uncomment(std::move(line)); if(line.empty())continue;
        if(line.front()=='['&&line.back()==']'){sec=lower(trim(line.substr(1,line.size()-2)));continue;}
        auto eq=line.find('='); if(eq==std::string::npos)continue;
        std::string key=lower(trim(line.substr(0,eq))), val=trim(line.substr(eq+1));
        if(sec=="project2dfx"||sec=="project 2dfx"){
            if(key=="enabled"){to_bool(val,c.enabled);saw_any=true;}
            else if(key=="renderlodlights"){to_bool(val,c.lod_lights);c.enabled=true;saw_any=true;}
            else if(key=="coronaradiusmultiplier"){to_float(val,c.corona_radius_multiplier);c.enabled=true;saw_any=true;}
            else if(key=="coronafarclip"){to_float(val,c.corona_far_clip);c.enabled=true;saw_any=true;}
            else if(key=="skygfx"){to_bool(val,c.sky_gfx);c.enabled=true;saw_any=true;}
            else if(key=="performancelog")to_bool(val,c.performance_log);
            else if(key=="performanceloginterval"||key=="logintervalframes")to_u32(val,c.performance_log_interval);
        } else if(sec=="lodlights"||sec=="lod lights"){
            if(key=="enabled")to_bool(val,c.lod_lights);
            else if(key=="coronalimit")to_u32(val,c.corona_limit);
            else if(key=="radiusmultiplier")to_float(val,c.corona_radius_multiplier);
            else if(key=="farclip")to_float(val,c.corona_far_clip);
            else if(key=="intensity")to_float(val,c.corona_intensity);
            else if(key=="segments")to_u32(val,c.corona_segments);
            else if(key=="distancegrowth")to_bool(val,c.distance_growth);
        } else if(sec=="trafficlights"||sec=="traffic lights"){
            if(key=="enabled")to_bool(val,c.traffic_lights);
        } else if(sec=="blinkinglights"||sec=="blinking lights"){
            if(key=="enabled")to_bool(val,c.blinking_lights);
        } else if(sec=="skygfx"){
            if(key=="enabled")to_bool(val,c.sky_gfx);
            else if(key=="starsperside")to_u32(val,c.stars_per_side);
            else if(key=="smalleststarssize")to_float(val,c.smallest_star);
            else if(key=="middlestarssize")to_float(val,c.middle_star);
            else if(key=="biggeststarssize")to_float(val,c.biggest_star);
            else if(key=="biggeststarschance")to_float(val,c.biggest_star_chance);
            else if(key=="seed")to_u32(val,c.star_seed);
        } else if(sec=="heliheight"||sec=="heli height"){
            if(key=="enabled")to_bool(val,c.heli_height);
            else if(key=="height"||key=="maxheight")to_float(val,c.heli_max_height);
        }
    }
    if(!saw_any && !c.enabled) return c;
    c.corona_radius_multiplier=std::clamp(c.corona_radius_multiplier,0.05f,16.0f);
    c.corona_far_clip=std::clamp(c.corona_far_clip,50.0f,10000.0f);
    c.corona_intensity=std::clamp(c.corona_intensity,0.0f,8.0f);
    c.corona_segments=std::clamp(c.corona_segments,6u,24u);
    c.stars_per_side=std::clamp(c.stars_per_side,0u,1000u);
    c.smallest_star=std::clamp(c.smallest_star,0.01f,5.0f);
    c.middle_star=std::clamp(c.middle_star,c.smallest_star,5.0f);
    c.biggest_star=std::clamp(c.biggest_star,c.middle_star,8.0f);
    c.biggest_star_chance=std::clamp(c.biggest_star_chance,0.0f,100.0f);
    c.heli_max_height=std::clamp(c.heli_max_height,80.0f,10000.0f);
    c.performance_log_interval=std::clamp(c.performance_log_interval,30u,3600u);
    return c;
}

void build_stars(){
    for(auto &v:g_stars)v.clear();
    std::mt19937 rng(g_cfg.star_seed);
    std::uniform_real_distribution<float> unit(0.0f,1.0f), sx(-95.0f,95.0f), sy(-95.0f,95.0f), side_y(-33.25f,95.0f);
    for(std::size_t side=0;side<g_stars.size();++side){
        g_stars[side].reserve(g_cfg.stars_per_side);
        for(std::uint32_t i=0;i<g_cfg.stars_per_side;++i){
            const float u=unit(rng);
            const float maxs=(u > (1.0f-g_cfg.biggest_star_chance*0.01f))?g_cfg.biggest_star:g_cfg.middle_star;
            const float size=(g_cfg.smallest_star + unit(rng)*(maxs-g_cfg.smallest_star))*0.8f;
            g_stars[side].push_back({sx(rng),side==4?sy(rng):side_y(rng),size});
        }
    }
}

bool inverse_camera_position(const CameraSnapshot &c, Vec3 &out){
    const auto&m=c.view;
    const float a=m[0],b=m[3],cc=m[6], d=m[1],e=m[4],f=m[7], g=m[2],h=m[5],i=m[8];
    const float det=a*(e*i-f*h)-b*(d*i-f*g)+cc*(d*h-e*g);
    if(!std::isfinite(det)||std::fabs(det)<1e-8f)return false;
    const float inv=1.0f/det;
    const float r00=(e*i-f*h)*inv, r01=(cc*h-b*i)*inv, r02=(b*f-cc*e)*inv;
    const float r10=(f*g-d*i)*inv, r11=(a*i-cc*g)*inv, r12=(cc*d-a*f)*inv;
    const float r20=(d*h-e*g)*inv, r21=(b*g-a*h)*inv, r22=(a*e-b*d)*inv;
    out.x=-(r00*m[9]+r01*m[10]+r02*m[11]);
    out.y=-(r10*m[9]+r11*m[10]+r12*m[11]);
    out.z=-(r20*m[9]+r21*m[10]+r22*m[11]);
    return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
}

bool original_project2dfx_camera(psprecomp::GuestMemory &memory,
                                 std::array<float,12> &view,Vec3 &position){
    constexpr std::uint32_t sprite_view_matrix=kTheCamera+0x0A60u;
    if(!memory.contains(kCameraPosition,12u)||
       !memory.contains(sprite_view_matrix,64u))return false;
    std::array<float,16> source{};
    for(std::size_t i=0;i<source.size();++i)
        source[i]=std::bit_cast<float>(memory.load32(
            sprite_view_matrix+std::uint32_t(i*4u)));
    position={
        std::bit_cast<float>(memory.load32(kCameraPosition+0u)),
        std::bit_cast<float>(memory.load32(kCameraPosition+4u)),
        std::bit_cast<float>(memory.load32(kCameraPosition+8u))};
    for(const float value:source)
        if(!std::isfinite(value)||std::fabs(value)>1.0e6f)return false;
    if(!std::isfinite(position.x)||!std::isfinite(position.y)||
       !std::isfinite(position.z)||std::fabs(position.x)>1.0e6f||
       std::fabs(position.y)>1.0e6f||std::fabs(position.z)>1.0e6f)return false;
    view={source[0],source[1],source[2],
          source[4],source[5],source[6],
          source[8],source[9],source[10],
          source[12],source[13],source[14]};
    const float det=view[0]*(view[4]*view[8]-view[7]*view[5])-
                    view[3]*(view[1]*view[8]-view[7]*view[2])+
                    view[6]*(view[1]*view[5]-view[4]*view[2]);
    return std::isfinite(det)&&std::fabs(det)>1.0e-6f;
}

bool same_camera(const CameraSnapshot &c,const std::array<float,12>&view,
                 const std::array<float,16>&projection,float sx,float sy,
                 float sz,float cx,float cy,float cz,float ox,float oy,
                 float clip_x) noexcept{
    return c.view==view&&c.projection==projection&&c.scale_x==sx&&
        c.scale_y==sy&&c.scale_z==sz&&c.center_x==cx&&c.center_y==cy&&
        c.center_z==cz&&c.offset_x==ox&&c.offset_y==oy&&
        c.clip_x_scale==clip_x;
}

std::uint64_t occluding_weight(const CameraSnapshot &c) noexcept{
    std::uint64_t result=0u;
    for(const auto weight:c.depth_function_weights)result+=weight;
    return result;
}

bool better_camera(const CameraSnapshot &candidate,
                   const CameraSnapshot &current) noexcept{
    if(!candidate.valid)return false;
    if(!current.valid)return true;
    const auto candidate_depth=occluding_weight(candidate);
    const auto current_depth=occluding_weight(current);
    return candidate_depth!=current_depth?candidate_depth>current_depth:
        candidate.weight>current.weight;
}

struct Projected {float x{},y{},z{},radius_x{},radius_y{},depth{};};
bool project_point(psprecomp::GuestMemory &memory,std::uint32_t guest_gp,
                   const std::array<float,12>&view,
                   const Vec3&p,float world_radius,Projected&o){
    const auto address=[guest_gp](std::int32_t offset){
        return guest_gp+static_cast<std::uint32_t>(offset);
    };
    constexpr std::array<std::int32_t,6> offsets={7792,-2988,7796,-8744,-8740,-16976};
    for(const auto offset:offsets)
        if(!memory.contains(address(offset),4u))return false;
    const auto load_float=[&](std::int32_t offset){
        return std::bit_cast<float>(memory.load32(address(offset)));
    };
    const auto load_int=[&](std::int32_t offset){
        return static_cast<std::int32_t>(memory.load32(address(offset)));
    };

    const Vec3 v{
        view[0]*p.x+view[3]*p.y+view[6]*p.z+view[9],
        view[1]*p.x+view[4]*p.y+view[7]*p.z+view[10],
        view[2]*p.x+view[5]*p.y+view[8]*p.z+view[11]};
    const float near_clip=load_float(7792)+load_float(-2988);
    const float far_clip=load_float(7796);
    if(!std::isfinite(v.z)||v.z<=near_clip||v.z>=far_clip)return false;
    const float viewport_x=float(load_int(-8744));
    const float viewport_y=float(load_int(-8740));
    const float projection_scale=load_float(-16976);
    if(!std::isfinite(viewport_x)||!std::isfinite(viewport_y)||
       !std::isfinite(projection_scale)||std::fabs(projection_scale)<1.0e-6f)
        return false;

    const float inverse_z=1.0f/v.z;
    // This is deliberately the exact coordinate space produced by the guest's
    // CSprite::CalcScreenCoors.  The matrix at TheCamera+0xA60 is not a plain
    // GE view matrix: together with these GP viewport factors it produces PSP
    // through-mode screen coordinates directly.  Applying the independently
    // observed GE viewport a second time made fixed world lights orbit/follow
    // the camera.
    const float screen_x=v.x*(viewport_x*inverse_z);
    const float screen_y=v.y*(viewport_y*inverse_z);
    const float adjusted_y=viewport_y*inverse_z*(70.0f/projection_scale);
    // 0x08AA83E8 scales the horizontal sprite size by viewportWidth / 480.
    // The previous /70 divisor came from the earlier projection-scale step and
    // made every corona about 6.86x too wide.
    const float size_x=adjusted_y*(viewport_x/480.0f);
    const float size_y=adjusted_y*(viewport_y/272.0f);
    o.x=screen_x;
    o.y=screen_y;
    o.radius_x=std::fabs(size_x*world_radius);
    o.radius_y=std::fabs(size_y*world_radius);
    o.depth=v.z;

    const float depth_max=std::bit_cast<float>(0x477FFA00u);
    const float depth_negative=std::bit_cast<float>(0xC77FFA00u);
    const float denominator=(far_clip-load_float(7792))*v.z;
    if(!std::isfinite(denominator)||std::fabs(denominator)<1.0e-12f)return false;
    const float mapped=((v.z-load_float(7792))*depth_negative*far_clip)/denominator+depth_max;
    if(!std::isfinite(mapped))return false;
    const auto depth_word=static_cast<std::int32_t>(std::trunc(mapped));
    o.z=float(static_cast<std::uint16_t>(depth_word));

    if(!std::isfinite(o.x)||!std::isfinite(o.y)||
       !std::isfinite(o.radius_x)||!std::isfinite(o.radius_y)||
       o.radius_x<=0.01f||o.radius_y<=0.01f)return false;
    return o.x+o.radius_x>=0.0f&&o.x-o.radius_x<=viewport_x&&
           o.y+o.radius_y>=0.0f&&o.y-o.radius_y<=viewport_y;
}

std::uint32_t pack_rgb(float r,float g,float b,float a=255.0f){
    auto c=[](float x){return std::uint32_t(std::clamp(std::lround(x),0l,255l));};
    return c(r)|(c(g)<<8u)|(c(b)<<16u)|(c(a)<<24u);
}

void emit_glow(std::vector<GeGpuVertex>&verts,const Projected&p,float radius_x,float radius_y,int r,int g,int b,float amount,std::uint32_t segments){
    if(amount<=0.001f||radius_x<=0.01f||radius_y<=0.01f)return;
    const float rr=std::clamp(float(r)*amount,0.0f,255.0f), gg=std::clamp(float(g)*amount,0.0f,255.0f), bb=std::clamp(float(b)*amount,0.0f,255.0f);
    const std::uint32_t center=pack_rgb(rr,gg,bb), edge=0xFF000000u;
    for(std::uint32_t s=0;s<segments;++s){
        const float a0=2.0f*kPi*float(s)/float(segments),a1=2.0f*kPi*float(s+1u)/float(segments);
        GeGpuVertex c{},v0{},v1{};c.x=p.x;c.y=p.y;c.z=p.z;c.rgba=center;
        v0.x=p.x+std::cos(a0)*radius_x;v0.y=p.y+std::sin(a0)*radius_y;v0.z=p.z;v0.rgba=edge;
        v1.x=p.x+std::cos(a1)*radius_x;v1.y=p.y+std::sin(a1)*radius_y;v1.z=p.z;v1.rgba=edge;
        verts.push_back(c);verts.push_back(v0);verts.push_back(v1);
    }
}

bool nighttime(std::uint32_t hour){return hour>=19u||hour<7u;}
float night_alpha(std::uint32_t h,std::uint32_t m){
    const float t=float(h*60u+m);
    if(h>=19u)return std::clamp(30.0f+(t-1140.0f)*(225.0f/300.0f),30.0f,255.0f);
    if(h<3u)return 255.0f;
    return std::clamp(255.0f-(t-180.0f)*(225.0f/240.0f),30.0f,255.0f);
}

bool blink_on(int mode,std::uint32_t time_ms){
    if(mode<=0||!g_cfg.blinking_lights)return true;
    std::uint32_t on=0,off=0;
    switch(mode){case 1:on=500;off=500;break;case 2:on=1000;off=1000;break;case 3:on=2000;off=2000;break;case 4:on=3000;off=3000;break;case 5:on=4000;off=4000;break;case 6:on=5000;off=5000;break;case 7:on=6000;off=4000;break;default:return true;}
    return time_ms%(on+off)<on;
}

bool traffic_visible(const project2dfx_data::LodLight&l,std::uint32_t minute){
    if(!g_cfg.traffic_lights)return false;
    const bool yellow=l.r>=250&&l.g>=100&&l.b<=100;
    if(yellow)return minute==9||minute==19||minute==29||minute==39||minute==49||minute==59;
    const bool red=l.r>=250&&l.g<100&&l.b==0, green=l.r==0&&l.g>=250&&l.b==0;
    const bool red_window=(minute<9)||(minute>=20&&minute<29)||(minute>=40&&minute<49);
    const bool green_window=(minute>9&&minute<19)||(minute>29&&minute<39)||(minute>49&&minute<59);
    const bool reverse=std::fabs(l.heading)>(kPi*0.5f);
    if(reverse)return (red&&red_window)||(green&&green_window);
    return (green&&red_window)||(red&&green_window);
}

std::pair<std::uint32_t,std::uint32_t> guest_clock(psprecomp::GuestMemory&mem,std::uint32_t gp,std::uint64_t vb){
    if(gp && mem.contains(gp+kGpClockHours,2u)){
        const auto h=std::uint32_t(mem.load8(gp+kGpClockHours));const auto m=std::uint32_t(mem.load8(gp+kGpClockMinutes));
        if(h<24u&&m<60u)return {h,m};
    }
    const std::uint64_t mins=(vb/60u)%1440u;return {std::uint32_t(mins/60u),std::uint32_t(mins%60u)};
}
std::uint32_t guest_timer(psprecomp::GuestMemory&mem,std::uint32_t gp,std::uint64_t vb){if(gp&&mem.contains(gp+kGpGameTimer,4u))return mem.load32(gp+kGpGameTimer);return std::uint32_t((vb*1000u)/60u);}

int dominant_depth_function(const CameraSnapshot &cam){
    const auto depth=std::max_element(
        cam.depth_function_weights.begin(),cam.depth_function_weights.end());
    if(depth==cam.depth_function_weights.end()||*depth==0u)return -1;
    return int(std::distance(cam.depth_function_weights.begin(),depth));
}

bool submit_vertices(const CameraSnapshot &cam,std::vector<GeGpuVertex>&verts){
    if(verts.empty())return false;
    const int depth_function=dominant_depth_function(cam);
    // Project2DFX is injected after the world pass. Without a depth-tested
    // world draw to establish the PSP comparison mode, drawing would turn the
    // coronas into an overlay over interiors, vehicles and characters.
    if(depth_function<0)return false;
    GeGpuDrawDescriptor d=cam.target;
    d.primitive=3u;d.vertex_count=std::uint32_t(verts.size());d.vertex_type=0u;d.through=true;
    d.texture_enabled=false;d.texture_address=0;d.texture_format=0;d.texture_content_signature=0;d.texture_use_alpha=false;d.texture_double_color=false;
    d.blend_enabled=true;d.blend_equation=0u;d.blend_source_factor=10u;d.blend_dest_factor=10u;d.blend_fix_source=0x00FFFFFFu;d.blend_fix_dest=0x00FFFFFFu;
    d.color_write_mask=0;d.alpha_test_enabled=false;
    d.depth_test_enabled=true;
    d.depth_function=std::uint32_t(depth_function);
    d.depth_write_enabled=false;d.fog_enabled=false;d.clear_mode=false;
    d.scissor_x0=0;d.scissor_y0=0;d.scissor_x1=std::max(0,int(d.framebuffer_stride?d.framebuffer_stride:480u)-1);d.scissor_y1=271;
    ge_gpu_backend_accumulate_color_triangles(d,verts);
    return true;
}

void render_lod_lights(psprecomp::GuestMemory &memory,
                       std::uint32_t guest_gp,const CameraSnapshot&cam,
                       const std::array<float,12>&view,const Vec3&camera,std::uint32_t hour,
                       std::uint32_t minute,std::uint32_t timer){
    if(!g_cfg.lod_lights||!nighttime(hour))return;
    const auto lights=project2dfx_data::vcs_lod_lights();
    if(lights.empty())return;
    g_frame_diagnostics.lights_scanned=lights.size();
    static thread_local std::vector<GeGpuVertex> verts;
    verts.clear();
    const std::size_t expected=std::min<std::size_t>(lights.size(),1024u)*g_cfg.corona_segments*3u;
    if(verts.capacity()<expected)verts.reserve(expected);
    const float base_alpha=night_alpha(hour,minute)/255.0f;
    std::uint32_t visible=0;
    for(const auto &l:lights){
        const bool traffic=std::fabs(l.custom_size_multiplier-0.45f)<0.0001f;
        if(traffic&&!traffic_visible(l,minute))continue;
        if(!traffic&&!blink_on(l.show_mode,timer))continue;
        const float dx=camera.x-l.x,dy=camera.y-l.y,dz=camera.z-l.z;const float ds=dx*dx+dy*dy+dz*dz;
        const float start=l.source_far_clip*0.5f;
        if(!l.no_distance && !(ds>start*start&&ds<g_cfg.corona_far_clip*g_cfg.corona_far_clip))continue;
        ++g_frame_diagnostics.lights_in_range;
        const float dist=std::sqrt(std::max(0.0f,ds));
        float radius=3.5f;
        if(!l.no_distance){const float den=std::max(1e-4f,l.source_far_clip-start);radius=std::clamp((dist-start)*(3.5f/den),0.0f,3.5f);}
        if(g_cfg.distance_growth)radius*=std::min(0.0025f*dist+0.25f,4.0f);
        radius*=l.custom_size_multiplier*g_cfg.corona_radius_multiplier;
        Projected p{};if(!project_point(memory,guest_gp,view,
                                       {l.x,l.y,l.z},radius,p))continue;
        ++g_frame_diagnostics.lights_projected;
        const float alpha=base_alpha*std::clamp(float(l.a)/255.0f,0.0f,1.0f)*g_cfg.corona_intensity;
        emit_glow(verts,p,p.radius_x,p.radius_y,l.r,l.g,l.b,alpha,g_cfg.corona_segments);
        ++g_frame_diagnostics.lights_emitted;
        if(++visible==g_cfg.corona_limit&&g_cfg.corona_limit!=0u)break;
    }
    g_frame_diagnostics.light_vertices=verts.size();
    (void)submit_vertices(cam,verts);
}

void render_stars(psprecomp::GuestMemory &memory,
                  std::uint32_t guest_gp,const CameraSnapshot&cam,
                  const std::array<float,12>&view,const Vec3&camera,
                  std::uint32_t h,std::uint32_t m){
    if(!g_cfg.sky_gfx)return;
    float a=0.0f;
    if(h>22u||h<5u)a=1.0f;else if(h==22u)a=float(m)/60.0f;else if(h==5u)a=float(60u-m)/60.0f;else return;
    if(a<=0.0f)return;
    static constexpr Vec3 base[5]={{100,0,10},{-100,0,10},{0,100,10},{0,-100,10},{0,0,95}};
    static thread_local std::vector<GeGpuVertex> verts;
    verts.clear();
    const std::size_t expected=std::size_t(g_cfg.stars_per_side)*5u*12u;
    if(verts.capacity()<expected)verts.reserve(expected);
    for(std::size_t side=0;side<5;++side){
        for(const auto&s:g_stars[side]){
            Vec3 p={camera.x+base[side].x,camera.y+base[side].y,camera.z+base[side].z};
            if(side<=1){p.y-=s.x;p.z+=s.y;}else if(side<=3){p.x-=s.x;p.z+=s.y;}else{p.x+=s.x;p.y+=s.y;}
            Projected q{};if(!project_point(memory,guest_gp,view,p,s.size,q))continue;
            ++g_frame_diagnostics.stars_projected;
            emit_glow(verts,q,q.radius_x,q.radius_y,255,255,255,a,
                      g_cfg.corona_segments);
        }
    }
    g_frame_diagnostics.star_vertices=verts.size();
    (void)submit_vertices(cam,verts);
}

void write_performance_sample(std::uint64_t vblank,const CameraSnapshot &cam,
                              std::uint32_t display_target){
    if(!g_cfg.performance_log||!g_performance_log||
       g_performance.frames<g_cfg.performance_log_interval)return;
    const GeGpuBackendReport backend=ge_gpu_backend_report();
    const auto delta=[](std::uint64_t now,std::uint64_t before){return now>=before?now-before:0u;};
    const double frames=double(std::max<std::uint64_t>(1u,g_performance.frames));
    g_performance_log
        << "vblank=" << vblank
        << " frames=" << g_performance.frames
        << " p2dfx_cpu_ms_avg=" << (double(g_performance.cpu_nanoseconds)/1.0e6/frames)
        << " lights_scanned_avg=" << (double(g_performance.totals.lights_scanned)/frames)
        << " lights_in_range_avg=" << (double(g_performance.totals.lights_in_range)/frames)
        << " lights_projected_avg=" << (double(g_performance.totals.lights_projected)/frames)
        << " lights_emitted_avg=" << (double(g_performance.totals.lights_emitted)/frames)
        << " light_vertices_avg=" << (double(g_performance.totals.light_vertices)/frames)
        << " stars_projected_avg=" << (double(g_performance.totals.stars_projected)/frames)
        << " star_vertices_avg=" << (double(g_performance.totals.star_vertices)/frames)
        << " depth_func=" << dominant_depth_function(cam)
        << " target=0x" << std::hex << (cam.target.framebuffer_address&0x001FFFF0u)
        << " display=0x" << display_target << std::dec
        << " backend_finish_ms_avg="
        << (double(delta(backend.perf_finish_frame_ns,g_previous_backend_report.perf_finish_frame_ns))/1.0e6/frames)
        << " backend_wait_ms_avg="
        << (double(delta(backend.perf_wait_for_frame_ns,g_previous_backend_report.perf_wait_for_frame_ns))/1.0e6/frames)
        << " backend_submit_ms_avg="
        << (double(delta(backend.perf_queue_submit_ns,g_previous_backend_report.perf_queue_submit_ns))/1.0e6/frames)
        << " game_vertices_delta=" << delta(backend.game_vertices,g_previous_backend_report.game_vertices)
        << '\n';
    g_performance_log.flush();
    g_previous_backend_report=backend;
    g_performance={};
}

void heli_height_1(psprecomp::Runtime&,psprecomp::AllegrexContext&ctx){
    // Replace the complete AOT basic block, not merely the LUI/MTC1 pair. The
    // kit's 0x08B01D58 continuation is an instruction inside the block and is
    // not a registered dispatch target in this recompilation.
    ctx.set_gpr(4,std::bit_cast<std::uint32_t>(g_cfg.heli_max_height));
    ctx.fpr[14]=g_cfg.heli_max_height;
    ctx.fpr[0]=ctx.fpr[12]<ctx.fpr[14]
        ? ctx.fpr[12]-ctx.fpr[13]
        : ctx.fpr[12]+ctx.fpr[13];
    ctx.pc=ctx.gpr[31];
}
void heli_height_2(psprecomp::Runtime&,psprecomp::AllegrexContext&ctx){
    // Same issue at 0x08B01DA8: emulate through the original return block.
    ctx.set_gpr(4,std::bit_cast<std::uint32_t>(g_cfg.heli_max_height));
    ctx.fpr[13]=g_cfg.heli_max_height;
    ctx.fpr[0]=ctx.fpr[12]<ctx.fpr[13]?-1.0f:ctx.fpr[12]-ctx.fpr[13];
    ctx.pc=ctx.gpr[31];
}

} // namespace

void install_project2dfx(psprecomp::Runtime &runtime,
                         const std::filesystem::path &ini_path,
                         std::uint32_t guest_scratch_base){
    g_runtime=&runtime;g_projection_scratch=guest_scratch_base;
    g_ini_path=ini_path;g_cfg=read_config(ini_path);g_camera_candidates.clear();g_hot_camera=nullptr;g_hot_camera_revision=0u;g_hot_camera_target=0u;g_last_rendered_vblank=std::numeric_limits<std::uint64_t>::max();
    // O draw distance tem [DrawDistance] Enabled proprio e nao faz parte do
    // Project2DFX original, entao instala antes do return abaixo. Estava depois
    // dele, o que amarrava um recurso independente ao outro sem dizer nada.
    install_draw_distance_patch(runtime,ini_path);
    if(!g_cfg.enabled){std::cerr<<"[Project2DFX] disabled\n";return;}
    if(g_cfg.performance_log){
#ifdef _WIN32
        _putenv_s("PSPRECOMP_GPU_TIMING_DIAG","1");
#else
        setenv("PSPRECOMP_GPU_TIMING_DIAG","1",0);
#endif
        const auto directory=ini_path.empty()?std::filesystem::current_path():ini_path.parent_path();
        g_performance_log.open(directory/"VCSProject2DFX.log",std::ios::trunc);
        if(g_performance_log){
            g_performance_log << "VCSNative Project2DFX performance log\n"
                              << "renderer=procedural_fallback radius_multiplier="
                              << g_cfg.corona_radius_multiplier
                              << " segments=" << g_cfg.corona_segments
                              << " far_clip=" << g_cfg.corona_far_clip
                              << " interval_frames=" << g_cfg.performance_log_interval
                              << " projection=guest_CSprite_CalcScreenCoors_08AA82D4"
                              << " camera=guest_TheCamera_08BC7E30\n";
            g_performance_log.flush();
        }
    }
    build_stars();
    if(g_cfg.lod_lights) project2dfx_data::initialize_vcs_lod_lights(ini_path);
    if(g_cfg.heli_height){runtime.register_function(kHeliHeight1,&heli_height_1,"vcs_p2dfx_heli_height_1");runtime.register_function(kHeliHeight2,&heli_height_2,"vcs_p2dfx_heli_height_2");}
    const auto n=project2dfx_data::vcs_lod_lights().size();
    std::cerr<<"[Project2DFX] enabled: lod="<<g_cfg.lod_lights<<" lights="<<n<<" traffic="<<g_cfg.traffic_lights<<" blink="<<g_cfg.blinking_lights<<" stars="<<g_cfg.sky_gfx<<" heli="<<g_cfg.heli_height<<"\n";
}

bool project2dfx_observe_camera_hot(const GeGpuDrawDescriptor&draw,std::uint32_t vertex_weight,std::uint64_t camera_state_revision) noexcept{
    if(!g_cfg.enabled||draw.through||draw.clear_mode||vertex_weight==0u)return true;
    const std::uint32_t target=draw.framebuffer_address&0x001FFFF0u;if(target==0u)return true;
    if(camera_state_revision==0u||g_hot_camera==nullptr||
       g_hot_camera_revision!=camera_state_revision||g_hot_camera_target!=target)return false;
    auto &c=*g_hot_camera;c.weight+=vertex_weight;
    const std::uint32_t depth_function=draw.depth_function&7u;
    if(draw.depth_test_enabled&&depth_function>=2u)
        c.depth_function_weights[depth_function]+=vertex_weight;
    return true;
}

void project2dfx_observe_camera(const std::array<float,12>&view,const std::array<float,16>&projection,float sx,float sy,float sz,float cx,float cy,float cz,float ox,float oy,float clip_x,const GeGpuDrawDescriptor&draw,std::uint32_t vertex_weight,std::uint64_t camera_state_revision) noexcept{
    if(project2dfx_observe_camera_hot(draw,vertex_weight,camera_state_revision))return;
    const std::uint32_t target=draw.framebuffer_address&0x001FFFF0u;
    if(!std::all_of(view.begin(),view.end(),[](float value){return std::isfinite(value);})||
       !std::all_of(projection.begin(),projection.end(),[](float value){return std::isfinite(value);})||
       !std::isfinite(sx)||!std::isfinite(sy)||!std::isfinite(sz)||
       !std::isfinite(cx)||!std::isfinite(cy)||!std::isfinite(cz)||
       !std::isfinite(ox)||!std::isfinite(oy))return;
    const float normalized_clip_x=(std::isfinite(clip_x)&&
        std::fabs(clip_x)>1e-6f)?clip_x:1.0f;
    auto &variants=g_camera_candidates[target];
    auto variant=std::find_if(variants.begin(),variants.end(),
        [&](const CameraSnapshot &candidate){
            return same_camera(candidate,view,projection,sx,sy,sz,cx,cy,cz,
                               ox,oy,normalized_clip_x);
        });
    if(variant==variants.end()){
        variants.emplace_back();
        variant=std::prev(variants.end());
        variant->view=view;variant->projection=projection;
        variant->scale_x=sx;variant->scale_y=sy;variant->scale_z=sz;
        variant->center_x=cx;variant->center_y=cy;variant->center_z=cz;
        variant->offset_x=ox;variant->offset_y=oy;
        variant->clip_x_scale=normalized_clip_x;
        variant->target=draw;variant->valid=true;
        if(g_runtime!=nullptr)
            variant->sprite_camera_valid=original_project2dfx_camera(
                g_runtime->memory(),variant->sprite_view,
                variant->camera_position);
    }
    auto &c=*variant;
    g_hot_camera=&c;g_hot_camera_revision=camera_state_revision;g_hot_camera_target=target;
    c.weight+=vertex_weight;
    const std::uint32_t depth_function=draw.depth_function&7u;
    // NEVER and ALWAYS do not provide occlusion. Counting ALWAYS as the
    // dominant world mode made some frames select a pipeline that let every
    // distant light pass through buildings.
    if(draw.depth_test_enabled&&depth_function>=2u)
        c.depth_function_weights[depth_function]+=vertex_weight;
}

void project2dfx_render_frame(psprecomp::GuestMemory &memory,std::uint32_t guest_gp,std::uint64_t vblank,std::uint32_t display_framebuffer) noexcept{
    if(!g_cfg.enabled||g_last_rendered_vblank==vblank||!ge_gpu_backend_graphics_ready())return;
    g_last_rendered_vblank=vblank;
    CameraSnapshot chosen{};
    const std::uint32_t display_target=display_framebuffer&0x001FFFF0u;
    // VCS renders the 3D world to an offscreen target (normally 0x00088000)
    // and only later composites it into the displayed framebuffer.  Therefore
    // an arbitrary projected draw on the display target must not outrank the
    // much heavier depth-tested world pass.
    for(const auto &[_,variants]:g_camera_candidates)
        for(const auto &candidate:variants)
            if(better_camera(candidate,chosen))chosen=candidate;
    g_camera_candidates.clear();
    // The original plugin runs from the world's corona-render pass. Reusing a
    // camera from an older frame makes the native port draw over pause menus
    // and loading screens, where no current 3D world pass exists.
    if(!chosen.valid)return;
    const auto begin=std::chrono::steady_clock::now();
    g_frame_diagnostics={};
    Vec3 cam=chosen.camera_position;
    std::array<float,12> sprite_view=chosen.sprite_view;
    // The normal path is the pass-synchronous snapshot above. Keep a fallback
    // only for unusual frames where the guest camera was not initialized yet
    // when the first eligible GE draw arrived.
    if(!chosen.sprite_camera_valid&&
       !original_project2dfx_camera(memory,sprite_view,cam))return;
    auto [h,m]=guest_clock(memory,guest_gp,vblank);const auto timer=guest_timer(memory,guest_gp,vblank);
    render_lod_lights(memory,guest_gp,chosen,sprite_view,cam,h,m,timer);
    render_stars(memory,guest_gp,chosen,sprite_view,cam,h,m);
    const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-begin).count();
    ++g_performance.frames;
    g_performance.cpu_nanoseconds+=std::uint64_t(std::max<std::int64_t>(0,elapsed));
    g_performance.totals.lights_scanned+=g_frame_diagnostics.lights_scanned;
    g_performance.totals.lights_in_range+=g_frame_diagnostics.lights_in_range;
    g_performance.totals.lights_projected+=g_frame_diagnostics.lights_projected;
    g_performance.totals.lights_emitted+=g_frame_diagnostics.lights_emitted;
    g_performance.totals.light_vertices+=g_frame_diagnostics.light_vertices;
    g_performance.totals.stars_projected+=g_frame_diagnostics.stars_projected;
    g_performance.totals.star_vertices+=g_frame_diagnostics.star_vertices;
    write_performance_sample(vblank,chosen,display_target);
}

} // namespace vcs
