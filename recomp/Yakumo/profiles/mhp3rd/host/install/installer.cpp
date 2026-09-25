#include "install/installer.hpp"

#include "app_paths.hpp"

#include "install/executable_preparation.hpp"
#include "install/game_identity.hpp"
#include "install/user_data.hpp"

#include "kernel/iso_image.hpp"
#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_jni.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <cerrno>
#endif

#include "psprecomp/sha256.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace mhp3rd::install {
namespace {

constexpr std::uint64_t kMiB = 1024u * 1024u;
// Room left over after copying, so the copy never fills the disk to the last byte.
constexpr std::uint64_t kFreeSpaceMargin = 64u * kMiB;

std::string display_name(const std::filesystem::path &path) { return path_to_utf8(path.filename()); }

std::string megabytes(std::uint64_t bytes) { return std::to_string((bytes + kMiB / 2u) / kMiB) + " MB"; }

std::uint32_t le16(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1u]) << 8u);
}

std::uint32_t le32(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return le16(data, offset) | (le16(data, offset + 2u) << 16u);
}

std::vector<std::uint8_t> read_file(IsoImage &iso, const IsoImage::Entry &entry) {
    std::vector<std::uint8_t> data(entry.size);
    if (iso.read(static_cast<std::uint64_t>(entry.lba) * IsoImage::kSectorSize, data) != data.size())
        throw InstallError("The disc image is incomplete: it ends before the files it lists. The file may be truncated "
                           "or damaged; make the image again.");
    return data;
}

// String values of a PARAM.SFO (PSF) file.
std::map<std::string, std::string> parse_sfo(const std::vector<std::uint8_t> &sfo) {
    std::map<std::string, std::string> values;
    if (sfo.size() < 20u || std::memcmp(sfo.data(), "\0PSF", 4u) != 0) return values;
    const std::uint32_t key_table = le32(sfo, 8u);
    const std::uint32_t data_table = le32(sfo, 12u);
    const std::uint32_t count = le32(sfo, 16u);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::size_t entry = 20u + static_cast<std::size_t>(i) * 16u;
        if (entry + 16u > sfo.size()) break;
        const std::size_t key = key_table + le16(sfo, entry);
        const std::uint32_t format = le16(sfo, entry + 2u);
        const std::uint32_t length = le32(sfo, entry + 4u);
        const std::size_t value = static_cast<std::size_t>(data_table) + le32(sfo, entry + 12u);
        if (key >= sfo.size() || value + length > sfo.size() || format != 0x0204u) continue;
        std::string name;
        for (std::size_t p = key; p < sfo.size() && sfo[p] != 0u; ++p) name.push_back(static_cast<char>(sfo[p]));
        std::string text(reinterpret_cast<const char *>(sfo.data() + value), length);
        if (const auto nul = text.find('\0'); nul != std::string::npos) text.resize(nul);
        values[name] = text;
    }
    return values;
}

struct Inspection {
    ImageInfo info;
    std::vector<std::uint8_t> eboot_bin;
};

Inspection inspect(const std::filesystem::path &path) {
    const std::string name = display_name(path);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
        throw InstallError("\"" + path_to_utf8(path) + "\" is not a file Yakumo can open.");
    {
        std::ifstream in(path, std::ios::binary);
        std::array<char, 4> magic{};
        if (!in.read(magic.data(), magic.size()))
            throw InstallError("\"" + name + "\" is empty or cannot be read.");
        if (std::memcmp(magic.data(), "CISO", 4u) == 0 || std::memcmp(magic.data(), "ZISO", 4u) == 0)
            throw InstallError("\"" + name + "\" is a compressed image. Yakumo needs an uncompressed .iso image of the "
                               "disc; decompress it first.");
    }

    std::optional<IsoImage> iso;
    try {
        iso.emplace(path);
    } catch (const std::exception &) {
        throw InstallError("\"" + name + "\" is not a disc image Yakumo can read. Choose an uncompressed .iso image "
                           "of the game's PSP disc.");
    }

    const auto sfo_entry = iso->find(kParamSfoPathOnDisc);
    if (!sfo_entry || sfo_entry->directory) {
        if (iso->find("PS3_GAME/PARAM.SFO"))
            throw InstallError("\"" + name + "\" is a PlayStation 3 disc image. Yakumo needs the PSP disc image of " +
                               kGameTitle + ": an .iso whose top level holds a PSP_GAME folder.");
        throw InstallError("\"" + name + "\" is not a PSP game disc image: it has no PSP_GAME/PARAM.SFO.");
    }
    const auto sfo = parse_sfo(read_file(*iso, *sfo_entry));
    const auto disc_id = sfo.find("DISC_ID");
    if (disc_id == sfo.end())
        throw InstallError("\"" + name + "\" has no disc id in PSP_GAME/PARAM.SFO, so it is not an image Yakumo "
                           "supports.");
    if (disc_id->second != kDiscId) {
        const auto title = sfo.find("TITLE");
        std::string what = "\"" + name + "\" is ";
        what += title != sfo.end() && !title->second.empty() ? title->second + " (" + disc_id->second + ")"
                                                              : "disc " + disc_id->second;
        what += ". Yakumo supports only " + std::string(kGameTitle) + ", the Japanese release with disc id " +
                kDiscIdDisplay + ".";
        if (disc_id->second == "ULJM05800")
            what += " This is the original PSP release of the game, which Yakumo does not support.";
        else
            what += " Other releases and regions are not supported.";
        throw InstallError(what);
    }

    const auto eboot_entry = iso->find(kExecutablePathOnDisc);
    if (!eboot_entry || eboot_entry->directory)
        throw InstallError("\"" + name + "\" has the right disc id but no PSP_GAME/SYSDIR/EBOOT.BIN. The image is "
                           "incomplete or modified; make it again from your disc.");
    Inspection result;
    result.eboot_bin = read_file(*iso, *eboot_entry);
    if (psprecomp::sha256_bytes(result.eboot_bin) != kEncryptedExecutableSha256)
        throw InstallError("\"" + name + "\" is " + kGameTitle + " (" + kDiscIdDisplay +
                           "), but its executable is not the version Yakumo supports. The image may be patched, "
                           "modified or damaged; make it again from an unmodified disc.");
    result.info.size_bytes = iso->size_bytes();
    return result;
}

void copy_with_progress(const std::filesystem::path &from, const std::filesystem::path &to, std::uint64_t total,
                        const ProgressFn &progress) {
    const std::string stage = "Copying the disc image";
    std::ifstream in(from, std::ios::binary);
    std::ofstream out(to, std::ios::binary | std::ios::trunc);
    if (!in) throw InstallError("Cannot read \"" + path_to_utf8(from) + "\".");
    if (!out) throw InstallError("Cannot write \"" + path_to_utf8(to) + "\".");
    std::vector<char> buffer(8u * kMiB);
    std::uint64_t done = 0u;
    progress(stage, 0u, total);
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize got = in.gcount();
        if (got <= 0) break;
        if (!out.write(buffer.data(), got))
            throw InstallError("Writing the copy of the disc image failed. Check that the disk has free space.");
        done += static_cast<std::uint64_t>(got);
        progress(stage, done, total);
    }
    out.close();
    if (!out || done != total) throw InstallError("Copying the disc image failed after " + megabytes(done) + ".");
}

void write_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    out.close();
    if (!out) throw InstallError("Cannot write \"" + path_to_utf8(path) + "\".");
}

bool same_file(const std::filesystem::path &a, const std::filesystem::path &b) {
    std::error_code ec;
    return std::filesystem::exists(b, ec) && std::filesystem::equivalent(a, b, ec);
}

void install_checked(const std::filesystem::path &image, ImageStorage storage, const std::filesystem::path &data_dir,
                     const ProgressFn &progress) {
    Inspection inspection = inspect(image);
    std::filesystem::create_directories(data_dir);

    const std::filesystem::path copied = data_dir / kCopiedImageFile;
    const std::filesystem::path copying = data_dir / (std::string(kCopiedImageFile) + ".part");
    const bool copy = storage == ImageStorage::Copy && !same_file(image, copied);
    if (copy) {
        const std::uint64_t needed = space_needed_to_copy(inspection.info);
        std::error_code ec;
        const auto space = available_space(data_dir);
        if (space && *space < needed)
            throw InstallError("Not enough free space to copy the disc image: it needs " + megabytes(needed) +
                               " in \"" + path_to_utf8(data_dir) + "\" and " + megabytes(*space) +
                               " is free. Free up space, or choose to use the image where it is.");
        try {
            copy_with_progress(image, copying, inspection.info.size_bytes, progress);
        } catch (...) {
            std::filesystem::remove(copying, ec);
            throw;
        }
        // Prepare from the copy, which also checks that it reads back intact.
        try {
            inspection = inspect(copying);
        } catch (const InstallError &) {
            std::filesystem::remove(copying, ec);
            throw InstallError("The copy of the disc image does not match the original. Check the disk and try "
                               "again.");
        }
    }

    const std::string stage = "Preparing the executable";
    progress(stage, 0u, 1u);
    const std::vector<std::uint8_t> executable =
        prepare_executable(inspection.eboot_bin, [&](std::uint64_t done, std::uint64_t total) {
            progress(stage, done, total);
        });
    const std::filesystem::path executable_path = data_dir / kExecutableFile;
    const std::filesystem::path executable_partial = data_dir / (std::string(kExecutableFile) + ".part");
    write_file(executable_partial, executable);

    // Past this point the installation completes; a cancel would leave the
    // copy renamed without its executable.
    UserSettings settings;
    if (storage == ImageStorage::Copy) {
        if (copy) std::filesystem::rename(copying, copied);
        settings.disc_image = kCopiedImageFile;
    } else {
        settings.disc_image = std::filesystem::absolute(image);
    }
    save_settings(data_dir, settings);
    std::filesystem::rename(executable_partial, executable_path);

    // A copy left from an earlier setup is no longer used.
    std::error_code ec;
    if (storage == ImageStorage::InPlace && !same_file(image, copied)) std::filesystem::remove(copied, ec);
}

} // namespace

ImageInfo check_image(const std::filesystem::path &path) { return inspect(path).info; }

void install(const std::filesystem::path &image, ImageStorage storage, const std::filesystem::path &data_dir,
             const ProgressFn &progress) {
    try {
        install_checked(image, storage, data_dir, progress);
    } catch (const InstallError &) {
        throw;
    } catch (const InstallCancelled &) {
        std::error_code ec;
        std::filesystem::remove(data_dir / (std::string(kExecutableFile) + ".part"), ec);
        throw;
    } catch (const std::filesystem::filesystem_error &e) {
        throw InstallError("Setup could not write to \"" + path_to_utf8(data_dir) + "\": " + e.code().message() + ".");
    } catch (const std::exception &e) {
        throw InstallError(std::string("Setup failed: ") + e.what() + ".");
    }
}

#if defined(MHP3RD_ANDROID_APP)
bool is_document_uri(const std::filesystem::path &image) { return path_to_utf8(image).rfind("content://", 0) == 0; }

std::filesystem::path copy_image_document(const std::string &uri, const std::filesystem::path &data_dir,
                                          const ProgressFn &progress) {
    const int fd = android::open_document(uri, "r");
    if (fd < 0) throw InstallError("Android would not let Yakumo read that file. Choose it again.");
    struct stat info {};
    const std::uint64_t size = ::fstat(fd, &info) == 0 ? static_cast<std::uint64_t>(info.st_size) : 0u;
    // The copy, with room to spare for the executable prepared from it.
    const std::uint64_t needed = size + kFreeSpaceMargin;
    if (const auto space = available_space(data_dir); space && size != 0u && *space < needed) {
        ::close(fd);
        throw InstallError("Not enough free space to copy the disc image: it needs " + megabytes(needed) + " and " +
                           megabytes(*space) + " is free on this device.");
    }
    std::error_code ec;
    std::filesystem::create_directories(data_dir, ec);
    const std::filesystem::path target = data_dir / kCopiedImageFile;
    const std::filesystem::path partial = data_dir / (std::string(kCopiedImageFile) + ".part");
    const std::string stage = "Copying the disc image";
    try {
        std::ofstream out(partial, std::ios::binary | std::ios::trunc);
        std::vector<char> buffer(4u << 20);
        std::uint64_t done = 0u;
        progress(stage, 0u, size);
        for (;;) {
            const ssize_t got = ::read(fd, buffer.data(), buffer.size());
            if (got < 0 && errno == EINTR) continue;
            if (got < 0) throw InstallError("Reading the disc image failed. Choose it again.");
            if (got == 0) break;
            out.write(buffer.data(), got);
            if (!out)
                throw InstallError("Writing the copy of the disc image failed. Check that the device has free space.");
            done += static_cast<std::uint64_t>(got);
            progress(stage, done, size);
        }
        out.close();
        if (!out)
            throw InstallError("Writing the copy of the disc image failed. Check that the device has free space.");
    } catch (...) {
        ::close(fd);
        std::filesystem::remove(partial, ec);
        throw;
    }
    ::close(fd);
    std::filesystem::rename(partial, target, ec);
    if (ec) throw InstallError("Could not keep the copy of the disc image: " + ec.message() + ".");
    return target;
}
#endif

void InstallerUi::run_task(const std::string &, const std::function<void()> &work) { work(); }

bool run_installer(InstallerUi &ui, const std::filesystem::path &data_dir) {
    for (;;) {
        if (!ui.introduce(data_dir)) return false;
        for (;;) {
            auto image = ui.choose_image();
            if (!image) break;  // back to the introduction
            try {
#if defined(MHP3RD_ANDROID_APP)
                // A document that reached here as it is (the SDL dialogs'
                // picker returns one) is copied in first, as the setup
                // screens do before they return it.
                if (is_document_uri(*image)) {
                    const std::string uri = path_to_utf8(*image);
                    std::cout << "[setup] copying " << uri << std::endl;
                    ui.run_task("Copying the disc image", [&] {
                        image = copy_image_document(uri, data_dir,
                                                    [&ui](const std::string &stage, std::uint64_t done,
                                                          std::uint64_t total) { ui.progress(stage, done, total); });
                    });
                }
#endif
                ImageInfo info;
                ui.run_task("Checking the disc image", [&] { info = check_image(*image); });
                const auto storage = ui.choose_storage(*image, info, data_dir);
                if (!storage) continue;  // choose another image
                ui.run_task("Setting up", [&] {
                    install(*image, *storage, data_dir,
                            [&ui](const std::string &stage, std::uint64_t done, std::uint64_t total) {
                                ui.progress(stage, done, total);
                            });
                });
                ui.finished(data_dir);
                return true;
            } catch (const InstallCancelled &) {
                std::cerr << "Setup: cancelled\n";
            } catch (const InstallError &e) {
                std::cerr << "Setup: " << e.what() << "\n";
                if (!ui.offer_retry(e.what())) return false;
            }
        }
    }
}

std::optional<std::uint64_t> available_space(const std::filesystem::path &data_dir) {
    // The directory may not exist yet: ask about the nearest one that does.
    std::error_code ec;
    std::filesystem::path probe = std::filesystem::absolute(data_dir, ec);
    while (!probe.empty() && !std::filesystem::exists(probe, ec)) {
        if (probe == probe.parent_path()) break;
        probe = probe.parent_path();
    }
    const auto space = std::filesystem::space(probe, ec);
    if (ec) return std::nullopt;
    return space.available;
}

std::uint64_t space_needed_to_copy(const ImageInfo &info) { return info.size_bytes + kFreeSpaceMargin; }

namespace {
bool setup_on_exit = false;
bool restart_on_exit = false;
}

void request_restart_on_exit() { restart_on_exit = true; }
bool restart_requested_on_exit() { return restart_on_exit; }

int restart(char **argv) {
    std::cout.flush();
    std::cerr.flush();
#if defined(MHP3RD_ANDROID_APP)
    // An app cannot exec itself; Android starts it again instead.
    (void)argv;
    android::relaunch();
    std::cerr << "Cannot restart; start Yakumo again to load the imported save\n";
    return 1;
#endif
    const std::filesystem::path self = executable_path();
    const std::string program = self.empty() ? std::string(argv[0]) : self.string();
#if defined(_WIN32)
    const intptr_t result = _execv(program.c_str(), argv);
    (void)result;
#else
    execv(program.c_str(), argv);
#endif
    std::cerr << "Cannot restart " << program << "; start it again to load the imported save\n";
    return 1;
}

void request_setup_on_exit() { setup_on_exit = true; }
bool setup_requested_on_exit() { return setup_on_exit; }

int restart_for_setup(const char *program) {
    // Nothing buffered survives the exec.
    std::cout.flush();
    std::cerr.flush();
#if defined(MHP3RD_ANDROID_APP)
    // The next start runs the setup: a marker in the data directory stands
    // for --install, which an app is not started with.
    (void)program;
    { std::ofstream(user_data_directory() / kSetupMarkerFile) << "setup\n"; }
    android::relaunch();
    std::cerr << "Start Yakumo again to set up\n";
    return 1;
#endif
    // The player asked for the setup: a game directory chosen for this run
    // would make --install skip it.
#if defined(_WIN32)
    _putenv_s("MHP3RD_GAME_DIR", "");
    const intptr_t result = _execlp(program, program, "--install", nullptr);
    (void)result;
#else
    unsetenv("MHP3RD_GAME_DIR");
    execlp(program, program, "--install", static_cast<char *>(nullptr));
#endif
    std::cerr << "Cannot restart " << program << " for the setup; start it with --install\n";
    return 1;
}

void print_progress(const std::string &stage, std::uint64_t done, std::uint64_t total) {
    static std::string last_stage;
    static std::uint64_t last_step = 0u;
    const std::uint64_t percent = total == 0u ? 100u : done * 100u / total;
    const std::uint64_t step = percent / 5u;
    if (stage == last_stage && step == last_step) return;
    last_stage = stage;
    last_step = step;
    std::cout << stage << ": " << percent << "%";
    if (total > kMiB) std::cout << " (" << megabytes(done) << " of " << megabytes(total) << ")";
    std::cout << std::endl;
}

} // namespace mhp3rd::install
