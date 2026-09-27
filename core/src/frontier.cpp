#include "p3p3ds/frontier.hpp"
#include "psprecomp/decoder.hpp"
namespace p3p3ds {
FrontierProof validate_frontier(const psprecomp::GuestMemory &m,
    const std::vector<psprecomp::ExecutableRange> &ranges,
    const std::map<std::uint32_t, std::string> &seeds,
    const std::set<std::uint32_t> &owned, const std::set<std::uint32_t> &imports,
    std::uint32_t target, std::uint32_t caller, std::uint32_t word) {
    using namespace psprecomp;
    auto fail=[](const char *why) { return FrontierProof{false,why,0,0}; };
    auto executable=[&](std::uint32_t pc) { return is_executable_address(ranges,pc) && m.contains(pc,4); };
    if (!executable(target) || !executable(caller)) return fail("unaligned or outside executable file-backed range");
    if (!owned.contains(caller)) return fail("caller is not owned by existing AOT coverage");
    if (m.load32(caller)!=word || (word>>26)!=3 ||
        (((caller+4)&0xF0000000u)|((word&0x03FFFFFFu)<<2))!=target)
        return fail("executed edge is not matching direct jal");
    if (imports.contains(target) || imports.contains(target&~7u)) return fail("import stub");
    if (seeds.contains(target) || owned.contains(target)) return fail("target already owned: possible interior entry or stale build");
    if (!executable(caller+4) || decode_allegrex(m.load32(caller+4)).is_control_flow())
        return fail("invalid call delay slot");
    // Conservative boundary evidence: independent stack prologue saving RA,
    // or a leaf immediately following a completed JR-RA plus its delay slot.
    const auto first=decode_allegrex(m.load32(target));
    bool boundary=target>=8 && executable(target-8) && m.load32(target-8)==0x03E00008u &&
        !decode_allegrex(m.load32(target-4)).is_control_flow();
    if (first.kind==OpcodeKind::Addiu && first.rs==29 && first.rt==29 && first.immediate<0) {
        auto saves_ra=[&](const auto &d) {
            return d.kind==OpcodeKind::Sw && d.rs==29 && d.rt==31 && d.immediate>=0 && d.immediate < -first.immediate;
        };
        for (unsigned off=4; off<64 && executable(target+off); off+=4) {
            const auto d=decode_allegrex(m.load32(target+off));
            if (saves_ra(d)) boundary=true;
            if (d.is_control_flow()) {
                // An ordinary conditional branch always executes its slot.
                // ULUS-10512 08B1BF70/74 saves RA here. Never accept an annulled
                // branch-likely slot or a link instruction that overwrites RA.
                const bool ordinary=d.kind==OpcodeKind::Beq || d.kind==OpcodeKind::Bne ||
                    d.kind==OpcodeKind::Blez || d.kind==OpcodeKind::Bgtz || d.kind==OpcodeKind::Bltz || d.kind==OpcodeKind::Bgez;
                if(ordinary && executable(target+off+4) && saves_ra(decode_allegrex(m.load32(target+off+4)))) boundary=true;
                break;
            }
        }
    }
    if (!boundary) return fail("ambiguous boundary: no independent prologue or preceding return");
    auto all=seeds; all.emplace(target,"candidate");
    const auto cfg=analyze_function(target,m,ranges,all,8192);
    if (cfg.truncated || cfg.labels.empty()) return fail("empty or truncated CFG");
    bool terminal=false;
    auto inside=[&](std::uint32_t pc) {return cfg.labels.contains(pc);};
    for (auto pc:cfg.labels) {
        if (owned.contains(pc) || imports.contains(pc)) return fail("conflicting overlap with existing AOT/import");
        const auto d=decode_allegrex(m.load32(pc));
        if (d.kind==OpcodeKind::Unsupported || d.kind==OpcodeKind::Vfpu || d.kind==OpcodeKind::Syscall)
            return fail("unsupported instruction in candidate CFG");
        if (d.has_delay_slot()) {
            if (!executable(pc+4) || owned.contains(pc+4) || cfg.labels.contains(pc+4)) return fail("invalid or overlapping delay slot");
            const auto slot=decode_allegrex(m.load32(pc+4));
            if (slot.is_control_flow() || slot.kind==OpcodeKind::Unsupported || slot.kind==OpcodeKind::Vfpu)
                return fail("unsupported delay slot");
        }
        if (d.kind==OpcodeKind::Jalr || (d.kind==OpcodeKind::Jr && d.rs!=31)) return fail("unproven indirect transfer in candidate CFG");
        if (d.kind==OpcodeKind::Jr) { terminal=true; continue; }
        if (d.kind==OpcodeKind::J || d.kind==OpcodeKind::Jal) {
            const auto to=((pc+4)&0xF0000000u)|(d.target<<2);
            if (!executable(to)) return fail("direct transfer leaves executable range");
            if (d.kind==OpcodeKind::J) {
                if (!inside(to) && !seeds.contains(to)) return fail("unclosed direct jump");
                terminal |= seeds.contains(to);
            } else if (!inside(pc+8)) return fail("missing call continuation");
        } else if (d.has_delay_slot()) {
            const auto to=pc+4+static_cast<std::uint32_t>(static_cast<std::int32_t>(d.immediate)*4);
            if (!inside(to) || !inside(pc+8)) return fail("unclosed branch CFG");
        } else if (!inside(pc+4)) return fail("fallthrough leaves CFG");
    }
    if (!terminal) return fail("no proven return or tail-call");
    return {true,"executed direct jal; independent boundary; closed supported CFG; no overlap",cfg.labels.size(),cfg.basic_block_count};
}
}
