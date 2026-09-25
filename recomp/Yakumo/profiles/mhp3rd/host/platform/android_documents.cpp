#include "platform/android_documents.hpp"

#include "platform/android_jni.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <fstream>
#include <vector>

namespace mhp3rd::android {
namespace {

namespace fs = std::filesystem;

// A readable name for a picked tree: the last part of its document id
// ("primary:Download/saves" gives "Download/saves").
std::string describe_tree(const std::string &uri) {
    std::string decoded;
    for (std::size_t i = 0; i < uri.size(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.size()) {
            decoded += static_cast<char>(std::stoi(uri.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            decoded += uri[i];
        }
    }
    const std::size_t colon = decoded.rfind(':');
    return colon == std::string::npos ? decoded : decoded.substr(colon + 1);
}

bool copy_document_to_file(const std::string &uri, const fs::path &target, std::string &error) {
    const int fd = open_document(uri, "r");
    if (fd < 0) {
        error = "cannot read " + target.filename().string();
        return false;
    }
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    std::array<char, 1 << 16> buffer{};
    bool ok = static_cast<bool>(out);
    while (ok) {
        const ssize_t got = ::read(fd, buffer.data(), buffer.size());
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) {
            ok = got == 0;
            break;
        }
        out.write(buffer.data(), got);
        ok = static_cast<bool>(out);
    }
    ::close(fd);
    if (!ok) error = "cannot copy " + target.filename().string();
    return ok;
}

bool copy_file_to_document(const fs::path &source, const std::string &folder_uri, std::string &error) {
    const std::optional<std::string> created = create(folder_uri, source.filename().string(), false);
    if (!created) {
        error = "cannot create " + source.filename().string() + " in the chosen folder";
        return false;
    }
    const int fd = open_document(*created, "w");
    if (fd < 0) {
        error = "cannot write " + source.filename().string();
        return false;
    }
    std::ifstream in(source, std::ios::binary);
    std::array<char, 1 << 16> buffer{};
    bool ok = static_cast<bool>(in);
    while (ok && in) {
        in.read(buffer.data(), buffer.size());
        std::streamsize left = in.gcount();
        const char *at = buffer.data();
        while (left > 0) {
            const ssize_t put = ::write(fd, at, static_cast<std::size_t>(left));
            if (put < 0 && errno == EINTR) continue;
            if (put <= 0) {
                ok = false;
                break;
            }
            at += put;
            left -= put;
        }
    }
    ok = ::close(fd) == 0 && ok;
    if (!ok) error = "cannot write " + source.filename().string();
    return ok;
}

// Copies the files of a folder document (not its subfolders) into `target`.
bool copy_save_folder(const std::string &uri, const fs::path &target, std::string &error) {
    const auto entries = list_folder(uri);
    if (!entries) {
        error = "cannot read the chosen folder";
        return false;
    }
    std::error_code ec;
    fs::create_directories(target, ec);
    for (const Entry &entry : *entries)
        if (!entry.directory && !copy_document_to_file(entry.uri, target / entry.name, error)) return false;
    return true;
}

bool holds_param_sfo(const std::vector<Entry> &entries) {
    for (const Entry &entry : entries)
        if (!entry.directory && entry.name == "PARAM.SFO") return true;
    return false;
}

// Looks for save folders (those holding a PARAM.SFO) under a folder document
// and copies each into `savedata`/<name>: in the folder itself, its SAVEDATA
// and PSP folders, and one level of other folders, so a memory stick's root,
// an export ("MHP3rd saves <time>/PSP/SAVEDATA") or a folder holding one all
// work. The first save of a name wins.
bool find_and_copy(const std::string &uri, const std::string &name, const fs::path &savedata, int depth,
                   std::string &error) {
    const auto entries = list_folder(uri);
    if (!entries) return true;
    if (holds_param_sfo(*entries)) {
        std::error_code ec;
        if (fs::exists(savedata / name, ec)) return true;
        return copy_save_folder(uri, savedata / name, error);
    }
    if (depth >= 5) return true;
    for (const Entry &entry : *entries) {
        if (!entry.directory) continue;
        const bool obvious = entry.name == "PSP" || entry.name == "SAVEDATA" || name == "SAVEDATA";
        if ((obvious || depth < 2) && !find_and_copy(entry.uri, entry.name, savedata, depth + 1, error))
            return false;
    }
    return true;
}

bool copy_tree(const fs::path &local, const std::string &folder_uri, std::string &error) {
    const std::optional<std::string> made = create(folder_uri, local.filename().string(), true);
    if (!made) {
        error = "cannot create " + local.filename().string() + " in the chosen folder";
        return false;
    }
    std::error_code ec;
    for (const fs::directory_entry &entry : fs::directory_iterator(local, ec)) {
        if (entry.is_directory(ec)) {
            if (!copy_tree(entry.path(), *made, error)) return false;
        } else if (!copy_file_to_document(entry.path(), *made, error)) {
            return false;
        }
    }
    if (ec) error = "cannot read " + local.string();
    return !ec;
}

} // namespace

std::optional<PickedImport> pick_saves_to_import(const fs::path &staging) {
    const std::optional<std::string> tree = pick_folder();
    if (!tree) return std::nullopt;
    PickedImport picked;
    std::error_code ec;
    fs::remove_all(staging, ec);
    const std::string where = describe_tree(*tree);
    // The staged copy carries the picked folder's name, which the review
    // screen shows; the saves go into its SAVEDATA, where find_saves() looks.
    const fs::path root = staging / where.substr(where.rfind('/') + 1);
    picked.staged = root;
    fs::create_directories(root / "SAVEDATA", ec);
    const std::string root_uri = tree_root(*tree);
    if (!list_folder(root_uri)) {
        picked.error = "Android would not let Yakumo read that folder.";
        return picked;
    }
    std::string error;
    const std::string root_name = where.substr(where.rfind('/') + 1);
    if (!find_and_copy(root_uri, root_name, root / "SAVEDATA", 0, error))
        picked.error = "Copying from the chosen folder failed: " + error + ".";
    return picked;
}

std::optional<PickedExport> pick_folder_and_copy(const fs::path &local) {
    const std::optional<std::string> tree = pick_folder();
    if (!tree) return std::nullopt;
    PickedExport result;
    result.where = describe_tree(*tree);
    std::string error;
    if (!copy_tree(local, tree_root(*tree), error)) result.error = error;
    return result;
}

} // namespace mhp3rd::android
