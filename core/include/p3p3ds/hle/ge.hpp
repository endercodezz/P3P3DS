#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <deque>
#include <string>
namespace psprecomp { class Runtime; class GuestMemory; struct AllegrexContext; }
namespace p3p3ds { class KernelState; }
namespace p3p3ds::hle {
enum class GeStatus : std::uint32_t { Completed=0, Queued=1, Running=2, Stalled=3, Paused=4 };
struct GeListInfo {
    std::uint32_t id{}, list_address{}, pc{}, stall_address{}, opt_param{};
    std::uint32_t context_address{}, stack_count{}, stack_address{};
    std::int32_t callback_id{-1};
    GeStatus status{GeStatus::Queued};
    std::uint64_t commands{}, completions{};
    std::uint32_t previous{}, finish_token{};
};
struct GeState {
    std::array<std::uint32_t,256> registers{};
    std::array<std::array<std::uint32_t,128>,5> matrices{};
    std::array<unsigned,5> matrix_index{};
    std::uint32_t vertex{}, index{}, offset{};
    std::uint32_t relative(std::uint32_t arg) const { return ((((registers[0x10]&0xF0000)<<8)|arg)+offset)&0x0FFFFFFF; }
    std::uint32_t color_address() const { return 0x04000000u | (registers[0x9C]&0x1FFFF0); }
    std::uint32_t depth_address() const { return 0x04000000u | (registers[0x9E]&0x1FFFF0); }
    unsigned color_stride() const { return registers[0x9D]&0x7FC; }
    unsigned depth_stride() const { return registers[0x9F]&0x7FC; }
    unsigned format() const { return registers[0xD2]&3; }
};
struct GeCallback { std::uint32_t signal{}, signal_arg{}, finish{}, finish_arg{}; };
class GeManager {
public:
    static constexpr std::uint32_t kEdramBase=0x04000000u, kEdramSize=0x00200000u;
    std::uint32_t edram_base() const { return kEdramBase; }
    std::uint32_t edram_size() const { return kEdramSize; }
    std::int32_t enqueue_list(psprecomp::Runtime &, std::uint32_t, std::uint32_t, int, std::uint32_t, bool head=false);
    std::int32_t update_stall(psprecomp::Runtime &, int, std::uint32_t);
    std::int32_t sync(psprecomp::Runtime &, int id, int mode, bool all=false);
    void pump(psprecomp::Runtime &);
    std::int32_t set_callback(psprecomp::GuestMemory &, std::uint32_t);
    std::int32_t unset_callback(int);
    const GeListInfo *find(int id) const;
    const GeListInfo &last_list() const { return lists_.rbegin()->second; }
    std::uint64_t enqueue_count() const { return lists_.size(); }
    const GeState &state() const { return state_; }
    const std::map<int,GeListInfo> &lists() const { return lists_; }
    bool writer_observed() const { return writer_observed_; }
    std::uint32_t writer_pc() const { return writer_pc_; }
    void report() const;
private:
    GeState state_;
    std::map<int,GeListInfo> lists_;
    std::map<int,GeCallback> callbacks_;
    std::deque<int> queue_;
    int next_id_{1}, next_callback_{0};
    bool writer_observed_{};
    std::uint32_t writer_pc_{};
    void capture_writer(psprecomp::Runtime &, const GeListInfo &, std::uint32_t);
    void deliver_finish_callback(psprecomp::Runtime &, const GeListInfo &, std::uint32_t end_pc);
};
void register_ge_module(psprecomp::Runtime &, KernelState &);
} // namespace p3p3ds::hle
