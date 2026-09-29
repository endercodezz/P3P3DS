#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/audio.hpp"
#include "psprecomp/runtime.hpp"
#include <cstdint>
#include <iostream>

static int failures;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; ++failures; } } while(0)

int main() {
    psprecomp::Runtime runtime;
    runtime.frontier_diagnostics=true;
    p3p3ds::KernelState kernel;
    p3p3ds::hle::register_audio_module(runtime,kernel);
    auto &cpu=runtime.cpu();
    auto reserve=[&](std::int32_t requested,std::int32_t samples,std::int32_t format) {
        cpu.gpr[4]=static_cast<std::uint32_t>(requested);
        cpu.gpr[5]=static_cast<std::uint32_t>(samples);
        cpu.gpr[6]=static_cast<std::uint32_t>(format);
        cpu.gpr[31]=0x08B6426Cu;
        runtime.invoke_import("sceAudio",0x5EC81C55u,cpu);
        return cpu.gpr[2];
    };
    // ULUS-10512 first executed reserve: automatic channel, 448 stereo samples.
    CHECK(reserve(-1,448,0)==7u);
    CHECK(kernel.audio().channel(7).reserved);
    CHECK(kernel.audio().channel(7).sample_count==448u);
    CHECK(kernel.audio().channel(7).format==0u);
    CHECK(!runtime.events.empty() && runtime.events.back().type=="audio_ch_reserve");
    if(!runtime.events.empty()) {
        const auto &f=runtime.events.back().fields;
        CHECK(f.at("channel")==0xFFFFFFFFu);
        CHECK(f.at("samplecount")==448u);
        CHECK(f.at("format")==0u);
        CHECK(f.at("result")==7u);
        CHECK(f.at("caller")==0x08B64264u);
        CHECK(f.at("return_pc")==0x08B6426Cu);
    }
    CHECK(reserve(-1,256,0)==6u); // Second call in the same P3P function.
    CHECK(kernel.audio().channel(6).reserved);
    CHECK(reserve(7,448,0)==0x80260003u); // Hardware fixture: already reserved.
    CHECK(reserve(8,448,0)==0x80260003u);
    CHECK(reserve(0,0,0)==0x80260006u);
    CHECK(reserve(0,96,0)==0x80260006u);
    CHECK(reserve(0,65504,0)==0x80260006u);
    CHECK(reserve(0,448,1)==0x80260007u);
    CHECK(!kernel.audio().channel(0).reserved);
    CHECK(reserve(0,64,0x10)==0u); // Hardware fixture: mono format is valid.
    CHECK(kernel.audio().channel(0).format==0x10u);
    for(int i=1;i<=5;++i) CHECK(reserve(i,1024,0)==static_cast<std::uint32_t>(i));
    CHECK(reserve(-1,448,0)==0x80260005u); // No channels left.
    CHECK(!runtime.stopped());
    psprecomp::Runtime other_runtime;
    p3p3ds::KernelState other_kernel;
    p3p3ds::hle::register_audio_module(other_runtime,other_kernel);
    auto &other=other_runtime.cpu();
    other.gpr[4]=0xFFFFFFFFu;other.gpr[5]=448u;other.gpr[6]=0u;
    other_runtime.invoke_import("sceAudio",0x5EC81C55u,other);
    CHECK(other.gpr[2]==7u);
    other.gpr[4]=static_cast<std::uint32_t>(-9);other.gpr[5]=448u;other.gpr[6]=0u;
    other_runtime.invoke_import("sceAudio",0x5EC81C55u,other);
    CHECK(other.gpr[2]==6u); // The hardware fixture accepts negative values other than -1.
    CHECK(other_kernel.audio().channel(7).reserved);
    CHECK(kernel.audio().channel(7).reserved);
    std::cout<<"audio reservation failures="<<failures<<'\n';
    return failures?1:0;
}
