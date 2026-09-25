#include "vcs_project2dfx_lights.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace vcs::project2dfx_data {
namespace {
std::vector<LodLight> g_lights;
std::filesystem::path g_source;

std::string trim(std::string s){
    const auto first=s.find_first_not_of(" \t\r\n");if(first==std::string::npos)return {};
    const auto last=s.find_last_not_of(" \t\r\n");return s.substr(first,last-first+1);
}

bool parse_number(std::string token, double &out){
    token=trim(std::move(token));
    if(token.empty())return false;
    if(token.back()=='f'||token.back()=='F')token.pop_back();
    try{std::size_t n=0;out=std::stod(token,&n);return n==token.size()&&std::isfinite(out);}catch(...){return false;}
}

bool parse_initializer(std::string line,LodLight &l){
    for(auto comment=line.find("/*");comment!=std::string::npos;comment=line.find("/*")){
        const auto end=line.find("*/",comment+2u);
        if(end==std::string::npos){line.resize(comment);break;}
        line.erase(comment,end+2u-comment);
    }
    const auto a=line.find('{'),b=line.find('}',a==std::string::npos?0:a+1);
    if(a==std::string::npos||b==std::string::npos)return false;
    line=line.substr(a+1,b-a-1);
    std::array<double,12> v{};std::size_t i=0,pos=0;
    while(pos<=line.size()&&i<v.size()){
        const auto comma=line.find(',',pos);const auto part=line.substr(pos,comma==std::string::npos?std::string::npos:comma-pos);
        if(!parse_number(part,v[i]))return false;
        ++i;if(comma==std::string::npos)break;pos=comma+1;
    }
    if(i!=12)return false;
    l.x=float(v[0]);l.y=float(v[1]);l.z=float(v[2]);l.custom_size_multiplier=float(v[3]);l.source_far_clip=float(v[4]);l.heading=float(v[5]);
    l.r=int(v[6]);l.g=int(v[7]);l.b=int(v[8]);l.a=int(v[9]);l.show_mode=int(v[10]);l.no_distance=int(v[11]);
    return std::isfinite(l.x)&&std::isfinite(l.y)&&std::isfinite(l.z)&&l.source_far_clip>0.0f;
}

bool load_text_table(const std::filesystem::path &path){
    std::ifstream in(path);if(!in)return false;
    std::vector<LodLight> parsed;std::string line;bool inside=false;bool source_c=path.extension()==".c";
    while(std::getline(in,line)){
        if(source_c){
            if(!inside){if(line.find("aLodLights[]")!=std::string::npos)inside=true;continue;}
            if(line.find("};")!=std::string::npos)break;
        }
        LodLight l{};if(parse_initializer(line,l))parsed.push_back(l);
    }
    if(parsed.empty())return false;
    g_lights=std::move(parsed);g_source=path;return true;
}

bool load_binary_table(const std::filesystem::path &path){
    std::ifstream in(path,std::ios::binary);if(!in)return false;
    char magic[8]{};in.read(magic,8);if(in.gcount()!=8||std::string_view(magic,8)!="VCS2DFX1")return false;
    std::uint32_t count=0;in.read(reinterpret_cast<char*>(&count),4);if(!in||count==0||count>100000u)return false;
    struct Disk {float x,y,z,size,farclip,heading;std::int32_t r,g,b,a,show,nodist;};
    static_assert(sizeof(Disk)==48);
    std::vector<Disk> d(count);in.read(reinterpret_cast<char*>(d.data()),std::streamsize(d.size()*sizeof(Disk)));if(!in)return false;
    g_lights.clear();g_lights.reserve(count);
    for(const auto &x:d)g_lights.push_back({x.x,x.y,x.z,x.size,x.farclip,x.heading,x.r,x.g,x.b,x.a,x.show,x.nodist});
    g_source=path;return true;
}
}

bool initialize_vcs_lod_lights(const std::filesystem::path &ini_path) noexcept {
    try{
        g_lights.clear();g_source.clear();
        const auto dir=ini_path.empty()?std::filesystem::current_path():ini_path.parent_path();
        const std::array<std::filesystem::path,6> candidates={
            dir/"VCSProject2DFX_Lights.bin",
            dir/"Project2DFX"/"VCSProject2DFX_Lights.bin",
            std::filesystem::current_path()/"VCSProject2DFX_Lights.bin",
            dir/"lodl.c",
            dir/"Project2DFX"/"lodl.c",
            std::filesystem::current_path()/"lodl.c"};
        for(const auto&p:candidates){
            if(!std::filesystem::exists(p))continue;
            const bool ok=p.extension()==".bin"?load_binary_table(p):load_text_table(p);
            if(ok){std::cerr<<"[Project2DFX] loaded "<<g_lights.size()<<" LOD lights from "<<p.string()<<"\n";return true;}
        }
        std::cerr<<"[Project2DFX] WARNING: VCS LOD-light table not found; LOD Lights disabled until VCSProject2DFX_Lights.bin is installed\n";
        return false;
    }catch(const std::exception&e){std::cerr<<"[Project2DFX] light-table error: "<<e.what()<<"\n";g_lights.clear();g_source.clear();return false;}catch(...){g_lights.clear();g_source.clear();return false;}
}

std::span<const LodLight> vcs_lod_lights() noexcept {return g_lights;}
const std::filesystem::path &vcs_lod_light_source() noexcept {return g_source;}

} // namespace vcs::project2dfx_data
