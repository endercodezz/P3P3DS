// Checks for the save-data code that need no game data: the AES and CMAC
// primitives against their published test vectors, PARAM.SFO round trips,
// and the encryption, hashing and folder layout round trips.
//
//   mhp3rd_savedata_tests
//   mhp3rd_savedata_tests --check <save folder> <data file name> <game key, 32 hex digits> [plaintext out]
//
// The second form checks a save made elsewhere, for example one copied from
// a PSP: the PARAM.SFO hashes, the data file's hash, and that the decrypted
// data encrypts back to a file with the same hash.
#include "save_data/aes128.hpp"
#include "save_data/param_sfo.hpp"
#include "save_data/savedata_crypto.hpp"
#include "save_data/savedata_store.hpp"
#include "save_data/save_transfer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace mhp3rd::savedata;

namespace {

int failures = 0;

void check(bool condition, const char *what) {
    std::printf("%s %s\n", condition ? "ok  " : "FAIL", what);
    if (!condition) ++failures;
}

std::vector<std::uint8_t> from_hex(const std::string &hex) {
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) out.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    return out;
}

Block block(const std::string &hex) {
    Block b{};
    const auto bytes = from_hex(hex);
    std::copy(bytes.begin(), bytes.end(), b.begin());
    return b;
}

void test_aes() {
    // FIPS-197 appendix C.1.
    const Aes128 aes(block("000102030405060708090a0b0c0d0e0f"));
    const Block plain = block("00112233445566778899aabbccddeeff");
    const Block cipher = block("69c4e0d86a7b0430d8cdb78070b4c55a");
    check(aes.encrypt(plain) == cipher, "AES-128 encrypts the FIPS-197 vector");
    check(aes.decrypt(cipher) == plain, "AES-128 decrypts the FIPS-197 vector");
}

void test_cmac() {
    // RFC 4493 section 4.
    const Aes128 aes(block("2b7e151628aed2a6abf7158809cf4f3c"));
    const auto message = from_hex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                                  "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    check(cmac(aes, {}) == block("bb1d6929e95937287fa37d129b756746"), "CMAC of the empty message");
    check(cmac(aes, std::span(message).first(16)) == block("070a16b46b4d4144f79bdd9dd04a287c"), "CMAC of 16 bytes");
    check(cmac(aes, std::span(message).first(40)) == block("dfa66747de9ae63030ca32611497c827"), "CMAC of 40 bytes");
    check(cmac(aes, message) == block("51f0bebf7e3b9d92fc49741779363cfe"), "CMAC of 64 bytes");
}

void test_param_sfo() {
    ParamSfo sfo;
    sfo.set_string("TITLE", "Title", 128u);
    sfo.set_integer("PARENTAL_LEVEL", 7u);
    sfo.set_binary("SAVEDATA_PARAMS", std::vector<std::uint8_t>(128u, 0x5Au), 128u);
    const auto bytes = sfo.serialize();
    const auto parsed = ParamSfo::parse(bytes);
    check(parsed.has_value(), "PARAM.SFO parses what it serializes");
    check(parsed && parsed->string("TITLE") == "Title", "PARAM.SFO keeps strings");
    check(parsed && parsed->integer("PARENTAL_LEVEL") == 7u, "PARAM.SFO keeps integers");
    check(parsed && parsed->binary("SAVEDATA_PARAMS") && parsed->binary("SAVEDATA_PARAMS")->at(127) == 0x5Au,
          "PARAM.SFO keeps binary values");
    check(parsed && parsed->serialize() == bytes, "PARAM.SFO serializes identically after a round trip");
    const auto offset = sfo.data_offset("SAVEDATA_PARAMS");
    check(offset && bytes.at(*offset) == 0x5Au && bytes.at(*offset + 127u) == 0x5Au, "PARAM.SFO reports data offsets");
}

void test_encryption() {
    const Block key = block("00112233445566778899aabbccddeeff");
    std::vector<std::uint8_t> plain(5000u);
    for (std::size_t i = 0; i < plain.size(); ++i) plain[i] = static_cast<std::uint8_t>(i * 7u + 3u);
    const Block random = block("0f0e0d0c0b0a09080706050403020100");
    for (const CryptMode mode : {CryptMode::Mode1, CryptMode::Mode3, CryptMode::Mode5}) {
        const std::string name = "mode " + std::to_string(static_cast<int>(mode)) + ": ";
        const auto encrypted = encrypt_data(plain, mode, &key, random);
        check(encrypted.size() == kEncryptedHeaderSize + 5008u, (name + "encryption adds the header and pads to 16").c_str());
        check(!std::equal(plain.begin(), plain.end(), encrypted.begin() + kEncryptedHeaderSize),
              (name + "encrypted data differs from the plaintext").c_str());
        const auto decrypted = decrypt_data(encrypted, mode, &key);
        check(decrypted && std::equal(plain.begin(), plain.end(), decrypted->begin()) &&
                  std::all_of(decrypted->begin() + 5000, decrypted->end(), [](std::uint8_t b) { return b == 0u; }),
              (name + "decryption restores the plaintext").c_str());
        if (mode == CryptMode::Mode1) continue;
        Block other = key;
        other[0] ^= 1u;
        const auto wrong = decrypt_data(encrypted, mode, &other);
        check(wrong && !std::equal(plain.begin(), plain.end(), wrong->begin()),
              (name + "another game key does not decrypt it").c_str());
        check(data_file_hash(encrypted, mode, &key) != data_file_hash(encrypted, mode, &other),
              (name + "the file hash depends on the game key").c_str());
    }
}

void test_store() {
    const auto root = std::filesystem::temp_directory_path() / "mhp3rd_savedata_tests";
    std::filesystem::remove_all(root);
    SaveFiles files;
    files.game_name = "TEST00000";
    files.save_name = "SLOT";
    files.file_name = "DATA.BIN";
    files.key = block("0102030405060708090a0b0c0d0e0f10");
    SaveContents contents;
    contents.data = std::vector<std::uint8_t>(4096u, 0x42u);
    contents.title = "Title";
    contents.savedata_title = "Save";
    contents.detail = "Detail";
    contents.parental_level = 1u;
    contents.icon0 = {1, 2, 3};
    std::string error;
    check(write_save(root, files, contents, error), "a save is written");
    const auto folder = root / "PSP" / "SAVEDATA" / "TEST00000SLOT";
    check(std::filesystem::exists(folder / "PARAM.SFO") && std::filesystem::exists(folder / "DATA.BIN") &&
              std::filesystem::exists(folder / "ICON0.PNG"),
          "the save uses the PSP folder layout");
    std::ifstream in(folder / "PARAM.SFO", std::ios::binary);
    const std::vector<std::uint8_t> sfo_bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    const auto sfo = ParamSfo::parse(sfo_bytes);
    const auto offset = sfo ? sfo->data_offset("SAVEDATA_PARAMS") : std::nullopt;
    check(offset && verify_param_sfo(sfo_bytes, *offset), "PARAM.SFO carries valid hashes");
    const auto loaded = load_save(root, files);
    check(loaded.status == LoadStatus::Ok && loaded.contents.data == contents.data, "the save loads back");
    check(loaded.contents.title == "Title" && loaded.contents.detail == "Detail", "the titles load back");

    // A changed data file must be reported as broken.
    {
        std::fstream data(folder / "DATA.BIN", std::ios::binary | std::ios::in | std::ios::out);
        data.seekp(100);
        data.put('\x7f');
    }
    check(load_save(root, files).status == LoadStatus::Broken, "a modified data file is rejected");
    SaveFiles missing = files;
    missing.save_name = "OTHER";
    check(load_save(root, missing).status == LoadStatus::NoData, "a missing save reports no data");
    check(delete_save(root, files) && !std::filesystem::exists(folder), "a save is deleted");
    std::filesystem::remove_all(root);
}

std::vector<std::uint8_t> file_bytes(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// Writes a save the way the game does, under <memory_stick>/PSP/SAVEDATA.
void make_save(const std::filesystem::path &memory_stick, const std::string &game, const std::string &save,
               const Block &key, std::uint8_t fill) {
    SaveFiles files;
    files.game_name = game;
    files.save_name = save;
    files.file_name = "MHP3RD.BIN";
    files.key = key;
    SaveContents contents;
    contents.data = std::vector<std::uint8_t>(2048u, fill);
    contents.title = "Title";
    contents.icon0 = {1, 2, 3};
    std::string error;
    if (!write_save(memory_stick, files, contents, error)) std::printf("cannot write a test save: %s\n", error.c_str());
}

void test_transfer() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "mhp3rd_save_transfer_tests";
    fs::remove_all(root);
    const Block key = block("0102030405060708090a0b0c0d0e0f10");
    const Block wrong_key = block("f0e0d0c0b0a090807060504030201000");
    const fs::path source = root / "stick";
    const fs::path source_saves = source / "PSP" / "SAVEDATA";
    make_save(source, "ULJM05800", "", key, 0x11u);
    make_save(source, "ULJM05800", "QST", key, 0x22u);
    make_save(source, "ULUS10000", "", key, 0x33u);

    check(is_game_save_name("ULJM05800QST") && !is_game_save_name("ULJM05800X"), "the game's folder names");
    check(check_save_folder(source_saves / "ULJM05800", key).ok(), "a save of this game passes");
    check(check_save_folder(source_saves / "ULJM05800QST", key).name == "ULJM05800QST", "the save's name is read");
    const SaveCheck other = check_save_folder(source_saves / "ULUS10000", key);
    check(!other.ok() && other.other_game, "a save of another game is refused");
    check(!check_save_folder(source_saves / "ULJM05800", wrong_key).ok(), "a save made with another key is refused");
    check(!check_save_folder(source_saves / "ULJM05800", std::nullopt).ok(), "an encrypted save needs the key");
    check(!check_save_folder(root, key).ok(), "a folder without PARAM.SFO is refused");

    // A damaged copy: one byte of the data file changed.
    const fs::path damaged = root / "damaged" / "ULJM05800";
    fs::create_directories(damaged);
    for (const auto &entry : fs::directory_iterator(source_saves / "ULJM05800"))
        fs::copy_file(entry.path(), damaged / entry.path().filename());
    {
        std::fstream data(damaged / "MHP3RD.BIN", std::ios::binary | std::ios::in | std::ios::out);
        data.seekp(200);
        data.put('\x7f');
    }
    check(!check_save_folder(damaged, key).ok(), "a damaged data file is refused");
    // A damaged PARAM.SFO: the title changed, which its hashes cover.
    fs::copy_file(source_saves / "ULJM05800" / "MHP3RD.BIN", damaged / "MHP3RD.BIN",
                  fs::copy_options::overwrite_existing);
    check(check_save_folder(damaged, key).ok(), "the undamaged copy passes");
    {
        auto sfo_bytes = file_bytes(damaged / "PARAM.SFO");
        const auto title = std::search(sfo_bytes.begin(), sfo_bytes.end(), std::begin("Title"), std::end("Title") - 1);
        if (title != sfo_bytes.end()) *title = 't';
        std::ofstream(damaged / "PARAM.SFO", std::ios::binary)
            .write(reinterpret_cast<const char *>(sfo_bytes.data()), static_cast<std::streamsize>(sfo_bytes.size()));
    }
    check(!check_save_folder(damaged, key).ok(), "a damaged PARAM.SFO is refused");
    fs::remove(damaged / "MHP3RD.BIN");
    check(!check_save_folder(damaged, key).ok(), "a missing data file is refused");

    // What the player picks: one save, a SAVEDATA folder or a memory stick.
    check(find_saves(source_saves / "ULJM05800", key).size() == 1u, "a save folder is found as itself");
    const auto from_stick = find_saves(source, key);
    check(from_stick.size() == 3u, "a memory stick's saves are found under PSP/SAVEDATA");
    check(std::count_if(from_stick.begin(), from_stick.end(), [](const SaveCheck &c) { return c.ok(); }) == 2,
          "only this game's saves can be imported");
    check(find_saves(source / "PSP", key).size() == 3u, "a PSP folder's saves are found under SAVEDATA");
    fs::create_directories(root / "empty");
    check(find_saves(root / "empty", key).empty(), "a folder without saves has none");

    // Backup naming: a time, and a suffix when it is taken.
    const auto time = std::chrono::system_clock::from_time_t(1790000000);
    const std::string stamp = timestamp_for_path(time);
    check(stamp.size() == 19u && stamp[4] == '-' && stamp[10] == '_' && stamp.find(':') == std::string::npos,
          "backup folders are named by date and time, without colons");
    const fs::path dest = root / "installed";
    const fs::path dest_saves = dest / "PSP" / "SAVEDATA";
    const fs::path first_backup = backup_directory(dest_saves, time);
    check(first_backup == dest_saves / ".backup" / stamp, "backups go to SAVEDATA/.backup/<time>");
    fs::create_directories(first_backup);
    check(backup_directory(dest_saves, time) == dest_saves / ".backup" / (stamp + "-2"),
          "a second backup in the same second gets its own folder");
    fs::remove_all(dest_saves / ".backup");

    // Import into an empty installation, then replace it with another save.
    const ImportResult first = import_save(check_save_folder(source_saves / "ULJM05800", key), dest,
                                           backup_directory(dest_saves, time));
    check(first.ok && first.backup.empty(), "a save is imported where none was");
    SaveFiles files;
    files.game_name = "ULJM05800";
    files.file_name = "MHP3RD.BIN";
    files.key = key;
    check(load_save(dest, files).contents.data == std::vector<std::uint8_t>(2048u, 0x11u), "the imported save loads");
    const auto old_bytes = file_bytes(dest_saves / "ULJM05800" / "MHP3RD.BIN");

    const fs::path newer = root / "newer";
    make_save(newer, "ULJM05800", "", key, 0x44u);
    const fs::path backup_dir = backup_directory(dest_saves, time);
    const ImportResult second =
        import_save(check_save_folder(newer / "PSP" / "SAVEDATA" / "ULJM05800", key), dest, backup_dir);
    check(second.ok && second.backup == backup_dir / "ULJM05800", "a replaced save is moved to the backup folder");
    check(file_bytes(second.backup / "MHP3RD.BIN") == old_bytes && fs::exists(second.backup / "PARAM.SFO"),
          "the backup holds the replaced save unchanged");
    check(load_save(dest, files).contents.data == std::vector<std::uint8_t>(2048u, 0x44u), "the new save loads");
    check(!fs::exists(dest_saves / ".import-ULJM05800"), "no partial copy is left behind");
    check(!import_save(check_save_folder(dest_saves / "ULJM05800", key), dest, backup_dir).ok,
          "the save in use cannot be imported over itself");
    check(!import_save(other, dest, backup_dir).ok && !fs::exists(dest_saves / "ULUS10000"),
          "a refused save is not copied");

    // Export: a new folder laid out like a memory stick.
    const fs::path target = root / "export";
    fs::create_directories(target);
    const ExportResult exported = export_saves(dest, target, time);
    check(exported.ok && exported.exported.size() == 1u, "the installed saves are exported");
    const fs::path out = exported.folder / "PSP" / "SAVEDATA" / "ULJM05800";
    bool same = true;
    for (const auto &entry : fs::directory_iterator(dest_saves / "ULJM05800"))
        same = same && file_bytes(entry.path()) == file_bytes(out / entry.path().filename());
    check(same, "the export is identical to the installed save");
    check(export_saves(dest, target, time).folder != exported.folder, "a second export does not replace the first");
    check(!export_saves(root / "nothing", target, time).ok, "there is nothing to export without saves");

    // Backups: named by time, or plain names that replace only when asked.
    const fs::path backups = root / "backups";
    fs::create_directories(backups);
    check(saves_to_back_up(dest) == std::vector<std::string>{"ULJM05800"}, "the installed saves are backed up");
    const fs::path timed = backup_folder(backups, time);
    check(timed == backups / stamp, "a timed backup is a folder named by its time");
    const BackupResult timed_result = back_up_saves(dest, timed, false);
    check(timed_result.ok && file_bytes(timed / "ULJM05800" / "MHP3RD.BIN") ==
                                 file_bytes(dest_saves / "ULJM05800" / "MHP3RD.BIN"),
          "a timed backup copies the save");
    check(backup_folder(backups, time) == backups / (stamp + "-2"), "a second timed backup gets its own folder");
    check(backup_folder(backups, std::nullopt) == backups, "an untimed backup uses the folder as it is");
    check(backup_conflicts(dest, backups).empty(), "no earlier untimed backup is in the way");
    check(back_up_saves(dest, backups, false).ok, "an untimed backup is written");
    check(backup_conflicts(dest, backups) == std::vector<std::string>{"ULJM05800"},
          "an earlier untimed backup is found");
    // Make the installed save differ from the backup, then back up again.
    make_save(newer, "ULJM05800", "", key, 0x55u);
    check(import_save(check_save_folder(newer / "PSP" / "SAVEDATA" / "ULJM05800", key), dest,
                      backup_directory(dest_saves, time))
              .ok,
          "another save is imported");
    const auto earlier = file_bytes(backups / "ULJM05800" / "MHP3RD.BIN");
    check(!back_up_saves(dest, backups, false).ok && file_bytes(backups / "ULJM05800" / "MHP3RD.BIN") == earlier,
          "an earlier backup is not replaced without asking");
    check(back_up_saves(dest, backups, true).ok && file_bytes(backups / "ULJM05800" / "MHP3RD.BIN") ==
                                                       file_bytes(dest_saves / "ULJM05800" / "MHP3RD.BIN"),
          "an earlier backup is replaced when the player agrees");
    check(!fs::exists(backups / ".ULJM05800.partial"), "no partial backup is left behind");
    check(!back_up_saves(dest, dest_saves, true).ok && load_save(dest, files).status == LoadStatus::Ok,
          "a backup never replaces the save itself");

    fs::remove_all(root);
}

} // namespace

std::vector<std::uint8_t> read_all(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int check_save(const std::filesystem::path &folder, const std::string &file_name, const std::string &key_hex,
               const char *plain_out) {
    const Block key = block(key_hex);
    const auto sfo_bytes = read_all(folder / "PARAM.SFO");
    const auto file = read_all(folder / file_name);
    const auto sfo = ParamSfo::parse(sfo_bytes);
    check(sfo.has_value(), "PARAM.SFO parses");
    if (!sfo || file.empty()) return EXIT_FAILURE;
    check(sfo->serialize() == sfo_bytes, "PARAM.SFO has the layout this code writes");
    const auto offset = sfo->data_offset("SAVEDATA_PARAMS");
    const auto *params = sfo->binary("SAVEDATA_PARAMS");
    const auto mode = params != nullptr ? mode_from_flags(params->at(0)) : std::nullopt;
    std::printf("SAVEDATA_PARAMS flags 0x%02x\n", params != nullptr ? params->at(0) : 0u);
    check(mode.has_value(), "the save is encrypted in a known mode");
    if (!mode) return EXIT_FAILURE;
    check(offset && verify_param_sfo(sfo_bytes, *offset), "PARAM.SFO hashes match");
    const auto *list = sfo->binary("SAVEDATA_FILE_LIST");
    bool listed = false;
    for (std::size_t o = 0; list != nullptr && o + 32u <= list->size(); o += 32u) {
        if (std::string(reinterpret_cast<const char *>(&(*list)[o])) != file_name) continue;
        Block stored{};
        std::copy_n(list->begin() + static_cast<std::ptrdiff_t>(o + 13u), 16, stored.begin());
        listed = stored == data_file_hash(file, *mode, &key);
    }
    check(listed, "the data file matches its hash in SAVEDATA_FILE_LIST");
    const auto plain = decrypt_data(file, *mode, &key);
    check(plain.has_value(), "the data file decrypts");
    if (!plain) return EXIT_FAILURE;
    const auto again = encrypt_data(*plain, *mode, &key, block("00000000000000000000000000000000"));
    check(decrypt_data(again, *mode, &key) == plain, "the plaintext encrypts and decrypts back");
    if (plain_out != nullptr) {
        std::ofstream out(plain_out, std::ios::binary);
        out.write(reinterpret_cast<const char *>(plain->data()), static_cast<std::streamsize>(plain->size()));
    }
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
    if (argc >= 5 && std::string(argv[1]) == "--check")
        return check_save(argv[2], argv[3], argv[4], argc >= 6 ? argv[5] : nullptr);
    test_aes();
    test_cmac();
    test_param_sfo();
    test_encryption();
    test_store();
    test_transfer();
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
