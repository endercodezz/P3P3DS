#include "vcs_runtime_log.hpp"
#include "vcs_config.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace vcs {
namespace {

struct RuntimeLogState {
    std::mutex mutex;
    std::ofstream file;
    std::filesystem::path path;
    bool enabled{};
    bool flush_every_line{true};
};

RuntimeLogState &state() {
    static RuntimeLogState s;
    return s;
}

std::string timestamp_now() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return out.str();
}

}

void runtime_log_initialize(const VcsConfiguration &configuration) {
    RuntimeLogState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    s.enabled = configuration.diagnostics.log_to_file;
    s.flush_every_line = configuration.diagnostics.flush_every_line;
    s.path.clear();
    if (s.file.is_open()) s.file.close();
    if (!s.enabled) return;
    std::filesystem::path file_name = configuration.diagnostics.log_file;
    if (file_name.empty()) file_name = "VCSNative.log";
    s.path = configuration.executable_directory / file_name;
    s.file.open(s.path, std::ios::out | std::ios::trunc);
    if (!s.file) {
        s.enabled = false;
        s.path.clear();
        return;
    }
    s.file << "VCSNative runtime log\n";
    s.file << "stage=45.9-async-ge-cross-unit-hot-register-cache\n";
    s.file << "config=" << configuration.source_path.string() << '\n';
    s.file << "started=" << timestamp_now() << "\n\n";
    if (s.flush_every_line) s.file.flush();
}

void runtime_log_shutdown() noexcept {
    RuntimeLogState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    if (s.file.is_open()) {
        s.file << '\n' << '[' << timestamp_now() << "] shutdown\n";
        s.file.flush();
        s.file.close();
    }
    s.path.clear();
    s.enabled = false;
}

bool runtime_log_enabled() noexcept {
    RuntimeLogState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    return s.enabled && s.file.is_open();
}

std::filesystem::path runtime_log_path() {
    RuntimeLogState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    return s.path;
}

void runtime_log_line(std::string_view line) {
    RuntimeLogState &s = state();
    std::lock_guard<std::mutex> guard(s.mutex);
    if (!s.enabled || !s.file.is_open()) return;
    s.file << '[' << timestamp_now() << "] " << line << '\n';
    if (s.flush_every_line) s.file.flush();
}

void runtime_log_error(std::string_view category, std::string_view message) {
    std::ostringstream out;
    out << category << ": " << message;
    runtime_log_line(out.str());
}

} // namespace vcs
