#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include <iostream>
static int failures;
#define CHECK(x) do {if (!(x)) {std::cerr << __LINE__ << ": " << #x << '\n'; ++failures;}} while(0)
int main() {
    psprecomp::Runtime r; p3p3ds::KernelState k;
    const auto root = k.threads().init_root_thread("root",0x08800000,0x09FFFF00,0);
    p3p3ds::hle::register_all_hle_modules(r,k);
    auto &c=r.cpu();
    r.memory().store32(0x08800100,0x00646D75); // "umd"
    c.gpr[4]=0x08800100;c.gpr[5]=0x08800200;c.gpr[6]=0x12345678;
    r.invoke_import("ThreadManForUser",0xE81CAF8F,c);
    const auto uid=static_cast<std::int32_t>(c.gpr[2]);
    CHECK(uid>0 && uid!=root);
    const auto *cb=k.threads().get_callback(uid);
    CHECK(cb); if(cb) {
        CHECK(cb->uid==uid && cb->owner_thread_uid==root && cb->name=="umd");
        CHECK(cb->function==0x08800200 && cb->common_argument==0x12345678);
    }
    CHECK(!k.threads().get_thread(uid));CHECK(!k.threads().get_callback(root));
    const auto thread=k.threads().create_thread("later",0x08800000,32,512,0,0,r.memory());
    CHECK(thread>0 && thread!=uid);
    auto reg=[&](std::int32_t id) {c.gpr[4]=static_cast<std::uint32_t>(id);r.invoke_import("sceUmdUser",0xAEE7404D,c);return c.gpr[2];};
    // pspautotests/tests/umd/register.expected: Invalid, Zero, Valid, Twice, Zero.
    CHECK(k.umd().registered_callback()==0);
    CHECK(reg(-1)==0x80010016);CHECK(reg(0)==0x80010016);
    CHECK(reg(uid)==0);CHECK(reg(uid)==0);CHECK(reg(0)==0x80010016);
    CHECK(reg(root)==0x80010016);CHECK(reg(thread)==0x80010016);CHECK(reg(999)==0x80010016);
    CHECK(k.umd().registered_callback()==uid);
    const auto second=k.threads().create_callback("second",0x08800300,0);
    CHECK(second!=uid && second!=thread);CHECK(reg(second)==0);
    CHECK(k.umd().registered_callback()==second);
    p3p3ds::KernelState independent;CHECK(independent.umd().registered_callback()==0);
    auto check_medium=[&]() {r.invoke_import("sceUmdUser",0x46EBB729,c);return c.gpr[2];};
    CHECK(!k.umd().medium_present());CHECK(check_medium()==0);
    k.umd().set_medium_present(false);CHECK(check_medium()==0);
    k.umd().set_medium_present(true);CHECK(k.umd().medium_present());CHECK(check_medium()==1);
    CHECK(!independent.umd().medium_present());
    CHECK(k.umd().registered_callback()==second);
    independent.umd().set_medium_present(true);
    k.umd().set_medium_present(false);CHECK(!k.umd().medium_present());CHECK(check_medium()==0);
    CHECK(independent.umd().medium_present());CHECK(k.umd().registered_callback()==second);
    // uOFW mediaman.c::sceUmdActivate: exact alias, mode 1/2, invalid argument 0x80010016.
    constexpr std::uint32_t alias_ptr=0x08800120u;
    constexpr char alias[]="disc0:";
    for(std::size_t i=0;i<sizeof(alias);++i)r.memory().store8(alias_ptr+static_cast<std::uint32_t>(i),alias[i]);
    auto activate=[&](std::uint32_t mode,std::uint32_t ptr) {
        c.gpr[4]=mode;c.gpr[5]=ptr;c.gpr[31]=0x08AA1600u;
        r.invoke_import("sceUmdUser",0xC6183D47u,c);return c.gpr[2];
    };
    CHECK(!k.umd().activation_requested());
    CHECK(activate(0,alias_ptr)==0x80010016u);
    CHECK(activate(3,alias_ptr)==0x80010016u);
    CHECK(activate(1,0)==0x80010016u);
    CHECK(activate(1,0x00010000u)==0x80010016u);
    constexpr std::uint32_t wrong_ptr=0x08800140u;
    constexpr char wrong_alias[]="umd0:";
    for(std::size_t i=0;i<sizeof(wrong_alias);++i)r.memory().store8(wrong_ptr+static_cast<std::uint32_t>(i),wrong_alias[i]);
    CHECK(activate(1,wrong_ptr)==0x80010016u);
    for(std::uint32_t i=0;i<7u;++i)r.memory().store8(wrong_ptr+i,'x');
    CHECK(activate(1,wrong_ptr)==0x80010016u);
    CHECK(!k.umd().activation_requested());CHECK(!k.umd().medium_present());
    CHECK(k.umd().registered_callback()==second);
    k.umd().set_medium_present(true);
    CHECK(activate(1,alias_ptr)==0u); // Observed P3P mode and guest string.
    CHECK(k.umd().activation_requested());CHECK(k.umd().medium_present());
    CHECK(k.umd().registered_callback()==second);
    CHECK(activate(2,alias_ptr)==0u); // Also accepted by uOFW; no mount claim.
    CHECK(k.umd().activation_requested());CHECK(check_medium()==1u);
    CHECK(!independent.umd().activation_requested());
    psprecomp::Runtime second_runtime;
    p3p3ds::hle::register_umd_module(second_runtime,independent);
    constexpr std::uint32_t second_alias_ptr=0x08800120u;
    for(std::size_t i=0;i<sizeof(alias);++i)
        second_runtime.memory().store8(second_alias_ptr+static_cast<std::uint32_t>(i),alias[i]);
    auto &second_cpu=second_runtime.cpu();
    second_cpu.gpr[4]=1;second_cpu.gpr[5]=second_alias_ptr;
    second_runtime.invoke_import("sceUmdUser",0xC6183D47u,second_cpu);
    CHECK(second_cpu.gpr[2]==0u);CHECK(independent.umd().activation_requested());
    CHECK(independent.umd().medium_present());CHECK(independent.umd().registered_callback()==0);
    CHECK(k.umd().registered_callback()==second);
    CHECK(!independent.threads().get_callback(uid));CHECK(!r.stopped());
    std::cout<<"UMD callback failures="<<failures<<'\n';return failures?1:0;
}
