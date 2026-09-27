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
    CHECK(!independent.threads().get_callback(uid));CHECK(!r.stopped());
    std::cout<<"UMD callback failures="<<failures<<'\n';return failures?1:0;
}
