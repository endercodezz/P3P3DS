#include "p3p3ds/frontier.hpp"
#include "psprecomp/decoder.hpp"
namespace p3p3ds {
namespace {
bool conditional_branch(psprecomp::OpcodeKind kind) {
    using psprecomp::OpcodeKind;
    switch (kind) {
    case OpcodeKind::Beq: case OpcodeKind::Bne: case OpcodeKind::Beql: case OpcodeKind::Bnel:
    case OpcodeKind::Blez: case OpcodeKind::Bgtz: case OpcodeKind::Blezl: case OpcodeKind::Bgtzl:
    case OpcodeKind::Bltz: case OpcodeKind::Bgez: case OpcodeKind::Bltzl: case OpcodeKind::Bgezl:
    case OpcodeKind::Bltzal: case OpcodeKind::Bgezal: case OpcodeKind::Bltzall: case OpcodeKind::Bgezall:
    case OpcodeKind::Bc1f: case OpcodeKind::Bc1t: case OpcodeKind::Bc1fl: case OpcodeKind::Bc1tl:
    case OpcodeKind::Bvf: case OpcodeKind::Bvt: case OpcodeKind::Bvfl: case OpcodeKind::Bvtl:
        return true;
    default: return false;
    }
}
}
FrontierProof validate_frontier(const psprecomp::GuestMemory &m,
    const std::vector<psprecomp::ExecutableRange> &ranges,
    const std::map<std::uint32_t, std::string> &seeds,
    const std::set<std::uint32_t> &owned, const std::set<std::uint32_t> &imports,
    std::uint32_t target, FrontierOrigin origin) {
    using namespace psprecomp;
    auto fail=[](const char *why) { return FrontierProof{false,why,0,0}; };
    auto executable=[&](std::uint32_t pc) { return is_executable_address(ranges,pc) && m.contains(pc,4); };
    if (!executable(target)) return fail("unaligned or outside executable file-backed range");
    if (origin.kind==FrontierOriginKind::DirectJal) {
        if (!executable(origin.caller)) return fail("unaligned or outside executable file-backed range");
        if (!owned.contains(origin.caller)) return fail("caller is not owned by existing AOT coverage");
        if (m.load32(origin.caller)!=origin.word || (origin.word>>26)!=3 ||
            (((origin.caller+4)&0xF0000000u)|((origin.word&0x03FFFFFFu)<<2))!=target)
            return fail("executed edge is not matching direct jal");
        if (!executable(origin.caller+4) || decode_allegrex(m.load32(origin.caller+4)).is_control_flow())
            return fail("invalid call delay slot");
    } else if (origin.kind==FrontierOriginKind::ThreadEntry) {
        if (origin.thread_uid<=0 || origin.entry_pc!=target)
            return fail("invalid thread-entry provenance");
    } else return fail("unknown frontier origin");
    if (imports.contains(target) || imports.contains(target&~7u)) return fail("import stub");
    if (seeds.contains(target) || owned.contains(target)) return fail("target already owned: possible interior entry or stale build");
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
    // A HI/LO multiply can be scheduled before the stack frame. Require a
    // completed unconditional predecessor and the RA-saving frame immediately
    // after that one instruction; the incoming-edge audit below excludes a
    // shared interior block. This deliberately does not accept a JAL alone.
    bool prefixed_frame=false;
    if (!boundary && (first.kind==OpcodeKind::Mult || first.kind==OpcodeKind::Multu) &&
        target>=8 && executable(target-8) && executable(target-4) && executable(target+4) &&
        decode_allegrex(m.load32(target-8)).kind==OpcodeKind::J &&
        !decode_allegrex(m.load32(target-4)).is_control_flow()) {
        const auto frame=decode_allegrex(m.load32(target+4));
        if (frame.kind==OpcodeKind::Addiu && frame.rs==29 && frame.rt==29 && frame.immediate<0) {
            for (unsigned off=8;off<64 && executable(target+off);off+=4) {
                const auto d=decode_allegrex(m.load32(target+off));
                if (d.kind==OpcodeKind::Sw && d.rs==29 && d.rt==31 &&
                    d.immediate>=0 && d.immediate < -frame.immediate) prefixed_frame=true;
                if (d.is_control_flow()) break;
            }
        }
    }
    if (!boundary && !prefixed_frame) return fail("ambiguous boundary: no independent prologue or preceding return");
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
    if (prefixed_frame) {
        auto candidate_words=cfg.labels;
        for (auto pc:cfg.labels)
            if (decode_allegrex(m.load32(pc)).has_delay_slot()) candidate_words.insert(pc+4);
        // Scan relocated executable words for cross-entries into the candidate.
        // External calls to its entry are fine. External jumps need the usual
        // restored-RA/stack tail-call shape; branches or entries into its body
        // would instead imply shared ownership.
        for (const auto &range:ranges) for (std::uint32_t pc=range.start;pc+4<=range.end;pc+=4) {
            if (!m.contains(pc,4)) continue;
            const auto d=decode_allegrex(m.load32(pc));
            std::uint32_t to=0;
            if (d.kind==OpcodeKind::J || d.kind==OpcodeKind::Jal)
                to=((pc+4)&0xF0000000u)|(d.target<<2);
            else if (conditional_branch(d.kind))
                to=pc+4+static_cast<std::uint32_t>(static_cast<std::int32_t>(d.immediate)*4);
            else continue;
            if (!candidate_words.contains(to) || candidate_words.contains(pc)) continue;
            if (to!=target || conditional_branch(d.kind)) return fail("external edge into candidate body");
            if (d.kind==OpcodeKind::J) {
                if (pc<16 || !executable(pc+4)) return fail("unproven jump into candidate entry");
                const auto slot=decode_allegrex(m.load32(pc+4));
                bool restores_ra=false;
                for (unsigned back=4;back<=16 && executable(pc-back);back+=4) {
                    const auto prior=decode_allegrex(m.load32(pc-back));
                    restores_ra |= prior.kind==OpcodeKind::Lw && prior.rs==29 && prior.rt==31;
                }
                if (!restores_ra || slot.kind!=OpcodeKind::Addiu || slot.rs!=29 ||
                    slot.rt!=29 || slot.immediate<=0) return fail("unproven jump into candidate entry");
            }
        }
    }
    if (!terminal) return fail("no proven return or tail-call");
    const auto origin_name=origin.kind==FrontierOriginKind::ThreadEntry ? "thread-entry origin" : "executed direct jal";
    return {true,std::string(origin_name)+(prefixed_frame ? "; prefixed RA-saving frame; isolated closed CFG; no overlap" :
        "; independent boundary; closed supported CFG; no overlap"),cfg.labels.size(),cfg.basic_block_count};
}
}
