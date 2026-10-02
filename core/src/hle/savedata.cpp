// sceUtilitySavedata (see savedata.hpp). Offsets: SceUtilitySavedataParam in
// psp/pspsdk/src/utility/psputility_savedata.h; NIDs: psp/pspsdk/src/utility/sceUtility.S.
#include "p3p3ds/hle/savedata.hpp"

#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>

namespace fs = std::filesystem;

namespace p3p3ds::hle {
namespace {

// SceUtilitySavedataParam field offsets.
constexpr std::uint32_t kResult = 0x1C, kMode = 0x30, kGameName = 0x3C, kSaveName = 0x4C, kSaveNameList = 0x60,
                        kFileName = 0x64, kDataBuf = 0x74, kDataBufSize = 0x78, kDataSize = 0x7C, kSfoTitle = 0x80,
                        kSfoSavedataTitle = 0x100, kSfoDetail = 0x180, kSfoParental = 0x580, kIcon0 = 0x584, kIcon1 = 0x594,
                        kPic1 = 0x5A4, kSnd0 = 0x5B4, kMsFree = 0x5D0, kMsData = 0x5D4,
                        kUtilityData = 0x5D8;
// Results (hardware transcripts, pspautotests utility/savedata/*.expected).
constexpr std::uint32_t kResultCancel = 1u;              // list dialog closed without a choice
constexpr std::uint32_t kErrorLoadNoData = 0x80110307u;  // autoload.expected / loaddata.expected
// [INFERRED] busy / wrong-state code for Init/Shutdown out of sequence.
constexpr std::uint32_t kErrorBadState = 0x80110001u;

enum Mode : std::uint32_t {
    AutoLoad = 0, AutoSave = 1, Load = 2, Save = 3, ListLoad = 4, ListSave = 5, ListDelete = 6, Sizes = 8, AutoDelete = 9, Delete = 10,
};

std::string guest_string(psprecomp::GuestMemory &m, std::uint32_t at, std::uint32_t max) {
    std::string s;
    for (std::uint32_t i = 0; i < max && m.contains(at + i, 1); ++i) {
        const auto c = m.load8(at + i);
        if (c == 0) break;
        s += static_cast<char>(c);
    }
    return s;
}
void put_guest_string(psprecomp::GuestMemory &m, std::uint32_t at, const std::string &s, std::uint32_t max) {
    for (std::uint32_t i = 0; i < max; ++i) m.store8(at + i, i < s.size() ? static_cast<std::uint8_t>(s[i]) : 0u);
}

// ---- PARAM.SFO (PSF) ----------------------------------------------------------------
// Layout: "\0PSF", version 0x0101, key table, data table, 16-byte index entries
// (key offset u16, format u16: 0x0204 UTF-8 string / 0x0404 int32, length, max length, data offset).
std::vector<std::uint8_t> write_sfo(const std::map<std::string, std::string> &strings, const std::map<std::string, std::uint32_t> &ints) {
    struct Entry { std::string key; bool is_int; std::string s; std::uint32_t v; };
    std::vector<Entry> entries;
    for (const auto &[k, v] : strings) entries.push_back({k, false, v, 0});
    for (const auto &[k, v] : ints) entries.push_back({k, true, {}, v});
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.key < b.key; });
    std::vector<std::uint8_t> keys, data, index;
    auto put16 = [](std::vector<std::uint8_t> &b, std::uint32_t v) { b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); };
    auto put32 = [](std::vector<std::uint8_t> &b, std::uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xFF); };
    for (const auto &e : entries) {
        put16(index, static_cast<std::uint32_t>(keys.size()));
        keys.insert(keys.end(), e.key.begin(), e.key.end());
        keys.push_back(0);
        const auto data_at = static_cast<std::uint32_t>(data.size());
        if (e.is_int) {
            put16(index, 0x0404); put32(index, 4); put32(index, 4); put32(index, data_at);
            put32(data, e.v);
        } else {
            const auto len = static_cast<std::uint32_t>(e.s.size() + 1);
            const auto max = (len + 3u) & ~3u;
            put16(index, 0x0204); put32(index, len); put32(index, max); put32(index, data_at);
            data.insert(data.end(), e.s.begin(), e.s.end());
            data.resize(data_at + max, 0);
        }
    }
    while (keys.size() % 4) keys.push_back(0);
    std::vector<std::uint8_t> out = {0, 'P', 'S', 'F'};
    put32(out, 0x0101);
    const auto key_start = static_cast<std::uint32_t>(20 + index.size());
    put32(out, key_start);
    put32(out, key_start + static_cast<std::uint32_t>(keys.size()));
    put32(out, static_cast<std::uint32_t>(entries.size()));
    out.insert(out.end(), index.begin(), index.end());
    out.insert(out.end(), keys.begin(), keys.end());
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

std::map<std::string, std::string> read_sfo_strings(const fs::path &path) {
    std::map<std::string, std::string> out;
    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> d((std::istreambuf_iterator<char>(in)), {});
    if (d.size() < 20 || std::memcmp(d.data(), "\0PSF", 4) != 0) return out;
    auto le16 = [&](std::size_t o) { return static_cast<std::uint32_t>(d[o] | (d[o + 1] << 8)); };
    auto le32 = [&](std::size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (static_cast<std::uint32_t>(d[o + 3]) << 24); };
    const auto keys = le32(8), values = le32(12), count = le32(16);
    for (std::uint32_t i = 0; i < count && 20 + i * 16 + 16 <= d.size(); ++i) {
        const std::size_t e = 20 + i * 16;
        if (le16(e + 2) != 0x0204) continue;
        const std::size_t k = keys + le16(e), v = values + le32(e + 12), len = le32(e + 4);
        if (k >= d.size() || v + len > d.size()) continue;
        out[std::string(reinterpret_cast<const char *>(&d[k]))] =
            std::string(reinterpret_cast<const char *>(&d[v]), strnlen(reinterpret_cast<const char *>(&d[v]), len));
    }
    return out;
}

bool write_file(const fs::path &path, psprecomp::GuestMemory &m, std::uint32_t address, std::uint32_t size) {
    if (size == 0 || !m.contains(address, size)) return false;
    std::vector<std::uint8_t> bytes(size);
    m.copy_out(address, bytes);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

SavedataSlot describe(const fs::path &root, const std::string &game, const std::string &name, const std::string &file) {
    SavedataSlot slot;
    slot.name = name;
    const fs::path dir = root / (game + name);
    std::error_code ec;
    slot.exists = fs::exists(dir / file, ec);
    if (slot.exists) {
        const auto sfo = read_sfo_strings(dir / "PARAM.SFO");
        auto get = [&](const char *k) { const auto it = sfo.find(k); return it == sfo.end() ? std::string() : it->second; };
        slot.title = get("TITLE");
        slot.savedata_title = get("SAVEDATA_TITLE");
        slot.detail = get("SAVEDATA_DETAIL");
        slot.modified = fs::last_write_time(dir / file, ec);
    }
    return slot;
}

} // namespace

void register_savedata_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    auto &sd = kernel.savedata();

    runtime.register_hle("sceUtility", 0x50C4CD57u, [&sd](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // InitStart
        const auto p = ctx.gpr[4];
        if (sd.status_ != 0) { ctx.set_gpr(2, kErrorBadState); return; }
        if (!rt.memory().contains(p, 0x600)) { ctx.set_gpr(2, 0x80000023u); return; }
        sd.param_ = p;
        sd.mode_ = rt.memory().load32(p + kMode);
        sd.status_ = 1;
        sd.init_reported_ = sd.shutdown_reported_ = false;
        sd.waiting_ = false;
        sd.choice_ = -2;
        sd.dialog_.reset();
        rt.event("savedata_init", {{"mode", sd.mode_}, {"param", p}});
        ctx.set_gpr(2, 0u);
    });

    runtime.register_hle("sceUtility", 0x8874DBE0u, [&sd](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // GetStatus
        // Hardware reports INIT and FINISHED once each before moving on.
        if (sd.status_ == 1) {
            if (sd.init_reported_) sd.status_ = 2;
            sd.init_reported_ = true;
        } else if (sd.status_ == 4) {
            if (sd.shutdown_reported_) sd.status_ = 0;
            else { sd.shutdown_reported_ = true; ctx.set_gpr(2, 4u); return; }
        }
        ctx.set_gpr(2, static_cast<std::uint32_t>(sd.status_));
    });

    runtime.register_hle("sceUtility", 0xD4B95FFBu, [&sd, &kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // Update
        ctx.set_gpr(2, 0u);
        if (sd.status_ != 2) return;
        auto &m = rt.memory();
        const auto p = sd.param_;
        const std::string game = guest_string(m, p + kGameName, 13);
        const std::string file = guest_string(m, p + kFileName, 13);
        const bool list = sd.mode_ == ListLoad || sd.mode_ == ListSave || sd.mode_ == ListDelete;

        if (list && !sd.dialog_) {
            SavedataDialog dialog;
            dialog.kind = sd.mode_ == ListLoad ? SavedataDialog::Kind::Load
                        : sd.mode_ == ListSave ? SavedataDialog::Kind::Save : SavedataDialog::Kind::Delete;
            dialog.game = game;
            const auto names = m.load32(p + kSaveNameList);
            for (std::uint32_t i = 0; names && i < 100 && m.contains(names + i * 20u, 20); ++i) {
                const auto name = guest_string(m, names + i * 20u, 20);
                if (name.empty()) break;
                dialog.slots.push_back(describe(sd.root, game, name, file));
            }
            if (dialog.slots.empty()) dialog.slots.push_back(describe(sd.root, game, guest_string(m, p + kSaveName, 20), file));
            dialog.focus = 0;
            sd.dialog_ = std::move(dialog);
            const bool nothing_to_load = sd.dialog_->kind != SavedataDialog::Kind::Save &&
                std::none_of(sd.dialog_->slots.begin(), sd.dialog_->slots.end(), [](const SavedataSlot &s) { return s.exists; });
            if (nothing_to_load) sd.choice_ = -3; // the system UI only reports that there is no data
            else if (sd.auto_choice) sd.choice_ = sd.auto_choice(*sd.dialog_);
            else sd.waiting_ = true;
            rt.event("savedata_dialog", {{"mode", sd.mode_}, {"slots", sd.dialog_->slots.size()}, {"waiting", sd.waiting_ ? 1u : 0u}});
        }
        if (sd.waiting_) return;

        std::string save = guest_string(m, p + kSaveName, 20);
        std::uint32_t result = 0;
        if (list) {
            if (sd.choice_ == -3) result = kErrorLoadNoData;
            else if (sd.choice_ < 0 || sd.choice_ >= static_cast<int>(sd.dialog_->slots.size())) result = kResultCancel;
            else { save = sd.dialog_->slots[static_cast<std::size_t>(sd.choice_)].name; put_guest_string(m, p + kSaveName, save, 20); }
        }
        const fs::path dir = sd.root / (game + save);
        std::error_code ec;
        if (result == 0) {
            switch (sd.mode_) {
            case AutoLoad: case Load: case ListLoad: {
                std::ifstream in(dir / file, std::ios::binary);
                if (!in) { result = kErrorLoadNoData; break; }
                const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
                const auto cap = m.load32(p + kDataBufSize), buf = m.load32(p + kDataBuf);
                const auto n = static_cast<std::uint32_t>(std::min<std::size_t>(bytes.size(), cap));
                if (n && m.contains(buf, n)) m.copy_in(buf, std::span<const std::uint8_t>(bytes.data(), n));
                m.store32(p + kDataSize, n);
                break;
            }
            case AutoSave: case Save: case ListSave: {
                fs::create_directories(dir, ec);
                if (!write_file(dir / file, m, m.load32(p + kDataBuf), m.load32(p + kDataSize))) { result = 0x80110385u; break; } // [INFERRED] write failure
                const struct { std::uint32_t at; const char *name; } extras[] = {
                    {kIcon0, "ICON0.PNG"}, {kIcon1, "ICON1.PMF"}, {kPic1, "PIC1.PNG"}, {kSnd0, "SND0.AT3"}};
                for (const auto &x : extras) write_file(dir / x.name, m, m.load32(p + x.at), m.load32(p + x.at + 8));
                const auto sfo = write_sfo({{"CATEGORY", "MS"}, {"SAVEDATA_DIRECTORY", game + save},
                                            {"TITLE", guest_string(m, p + kSfoTitle, 0x80)},
                                            {"SAVEDATA_TITLE", guest_string(m, p + kSfoSavedataTitle, 0x80)},
                                            {"SAVEDATA_DETAIL", guest_string(m, p + kSfoDetail, 0x400)}},
                                           {{"PARENTAL_LEVEL", m.load8(p + kSfoParental)}});
                std::ofstream(dir / "PARAM.SFO", std::ios::binary).write(reinterpret_cast<const char *>(sfo.data()), static_cast<std::streamsize>(sfo.size()));
                break;
            }
            case Sizes: {
                // sizes.expected: clusterSize 0x8000, freeClusters capped at 0x7FFFF
                // ([INFERRED] cap), sizes in KB and as "N KB"/"N MB"/"N GB" strings.
                constexpr std::uint32_t kCluster = 0x8000;
                auto size_string = [](std::uint64_t kb) {
                    char s[8];
                    if (kb >= 1024u * 1024u) std::snprintf(s, sizeof s, "%u GB", static_cast<unsigned>(kb / (1024u * 1024u)));
                    else if (kb >= 1024u) std::snprintf(s, sizeof s, "%u MB", static_cast<unsigned>(kb / 1024u));
                    else std::snprintf(s, sizeof s, "%u KB", static_cast<unsigned>(kb));
                    return std::string(s);
                };
                auto clusters_of = [&](std::uint64_t bytes) { return static_cast<std::uint32_t>((bytes + kCluster - 1) / kCluster); };
                if (const auto free_info = m.load32(p + kMsFree); free_info && m.contains(free_info, 20)) {
                    fs::create_directories(sd.root, ec);
                    const auto avail = fs::space(sd.root, ec).available;
                    const auto clusters = static_cast<std::uint32_t>(std::min<std::uint64_t>(ec ? 0x7FFFFu : avail / kCluster, 0x7FFFFu));
                    m.store32(free_info, kCluster);
                    m.store32(free_info + 4, clusters);
                    m.store32(free_info + 8, clusters * (kCluster / 1024u));
                    put_guest_string(m, free_info + 12, size_string(static_cast<std::uint64_t>(clusters) * (kCluster / 1024u)), 8);
                }
                auto put_used = [&](std::uint32_t at, std::uint64_t bytes) { // SceUtilitySavedataUsedDataInfo
                    const auto clusters = clusters_of(bytes);
                    m.store32(at, clusters);
                    m.store32(at + 4, clusters * (kCluster / 1024u));
                    put_guest_string(m, at + 8, size_string(clusters * (kCluster / 1024u)), 8);
                    m.store32(at + 16, clusters * (kCluster / 1024u));
                    put_guest_string(m, at + 20, size_string(clusters * (kCluster / 1024u)), 8);
                };
                if (const auto need = m.load32(p + kUtilityData); need && m.contains(need, 28)) {
                    std::uint64_t bytes = m.load32(p + kDataSize);
                    for (const auto at : {kIcon0, kIcon1, kPic1, kSnd0}) bytes += m.load32(p + at + 8);
                    put_used(need, bytes + 0x1000); // + PARAM.SFO
                }
                if (const auto data = m.load32(p + kMsData); data && m.contains(data, 36 + 28)) {
                    const fs::path existing = sd.root / (guest_string(m, data, 13) + guest_string(m, data + 16, 20));
                    std::uint64_t bytes = 0;
                    if (fs::exists(existing, ec))
                        for (const auto &e : fs::directory_iterator(existing, ec)) bytes += fs::file_size(e.path(), ec);
                    if (bytes) put_used(data + 36, bytes);
                    else { result = kErrorLoadNoData; }
                }
                break;
            }
            case ListDelete: case AutoDelete: case Delete:
                if (!fs::exists(dir, ec)) { result = kErrorLoadNoData; break; }
                fs::remove_all(dir, ec);
                break;
            default:
                rt.stop("sceUtilitySavedata mode " + std::to_string(sd.mode_) + " not implemented");
                return;
            }
        }
        m.store32(p + kResult, result);
        ++sd.operations_;
        rt.event("savedata_done", {{"mode", sd.mode_}, {"result", result}}, game + save);
        sd.status_ = 3;
        (void)kernel;
    });

    runtime.register_hle("sceUtility", 0x9790B33Cu, [&sd](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // ShutdownStart
        if (sd.status_ != 3) { ctx.set_gpr(2, kErrorBadState); return; }
        sd.status_ = 4;
        sd.shutdown_reported_ = false;
        ctx.set_gpr(2, 0u);
    });
}

} // namespace p3p3ds::hle
