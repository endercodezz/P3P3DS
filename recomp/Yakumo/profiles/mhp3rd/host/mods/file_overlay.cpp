#include "mods/file_overlay.hpp"

#include "mods/mod_ini.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <system_error>

namespace mhp3rd::mods {

namespace fs = std::filesystem;

void FileOverlay::set(Resolution resolution) {
    resolution_ = std::move(resolution);
    cache_.clear();
}

std::optional<std::uint64_t> FileOverlay::size(FileId file) const {
    if (const auto found = resolution_.replacements.find(file); found != resolution_.replacements.end()) {
        std::error_code ec;
        const std::uintmax_t bytes = fs::file_size(found->second.path, ec);
        if (!ec) return static_cast<std::uint64_t>(bytes);
        return std::nullopt;
    }
    if (resolution_.patches.contains(file)) return game_.original_size(file);
    return std::nullopt;
}

std::map<FileId, std::uint64_t> FileOverlay::sizes() const {
    std::map<FileId, std::uint64_t> result;
    for (const auto &[file, source] : resolution_.replacements) {
        if (const auto bytes = size(file)) result[file] = *bytes;
    }
    for (const auto &[file, patches] : resolution_.patches) {
        if (!result.contains(file)) result[file] = game_.original_size(file);
    }
    return result;
}

std::shared_ptr<const FileContent> FileOverlay::content(FileId file) {
    if (const auto cached = cache_.find(file); cached != cache_.end()) return cached->second;
    if (!touches(file)) return nullptr;
    auto content = std::make_shared<FileContent>();
    if (const auto found = resolution_.replacements.find(file); found != resolution_.replacements.end()) {
        std::ifstream in(found->second.path, std::ios::binary);
        if (in) {
            content->bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            content->replaced = true;
        } else {
            content->problems.push_back("cannot read " + to_utf8(found->second.path) + "; the game's file is used");
        }
    }
    if (!content->replaced) content->bytes = game_.original(file);
    if (const auto found = resolution_.patches.find(file); found != resolution_.patches.end()) {
        for (const Resolution::Source &source : found->second) {
            PatchOutcome outcome = game_.patch(file, content->bytes, source.path);
            if (outcome.applied > 0u || !outcome.after_load.empty()) ++content->patches;
            for (MemoryWrite &write : outcome.after_load) content->after_load.push_back(std::move(write));
            for (std::string &problem : outcome.problems)
                content->problems.push_back(source.mod + ": " + std::move(problem));
        }
    }
    cache_[file] = content;
    return content;
}

} // namespace mhp3rd::mods
