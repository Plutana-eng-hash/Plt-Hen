#include "vypex/plugin_store.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

namespace vypex {
namespace {
constexpr const char* ROOT = "/data/Vypex-Cr_ck";
constexpr const char* PLUGIN_DIR = "/data/Vypex-Cr_ck/plugins";
constexpr const char* GHEN_CONFIG = "/data/GoldHEN/plugins.ini";
constexpr const char* GHEN_BACKUP = "/data/GoldHEN/plugins.ini.vypex.bak";
constexpr const char* GHEN_TEMP = "/data/GoldHEN/plugins.ini.vypex.tmp";

bool make_directory(const char* path) {
    if (::mkdir(path, 0777) == 0) return true;
    struct stat info{};
    return ::stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

bool ensure_directories() {
    return make_directory(ROOT) && make_directory(PLUGIN_DIR);
}

bool ends_with_sprx(const char* name) {
    const std::size_t n = std::strlen(name);
    return n > 5 && name[n - 5] == '.' &&
        std::tolower(static_cast<unsigned char>(name[n - 4])) == 's' &&
        std::tolower(static_cast<unsigned char>(name[n - 3])) == 'p' &&
        std::tolower(static_cast<unsigned char>(name[n - 2])) == 'r' &&
        std::tolower(static_cast<unsigned char>(name[n - 1])) == 'x';
}

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

bool read_lines(std::vector<std::string>& lines) {
    FILE* file = std::fopen(GHEN_CONFIG, "r");
    if (!file) return true;
    char line[1024];
    while (std::fgets(line, sizeof(line), file)) {
        lines.emplace_back(line);
        if (lines.size() > 4096) {
            std::fclose(file);
            return false;
        }
    }
    const bool ok = std::ferror(file) == 0;
    std::fclose(file);
    return ok;
}

bool parse_plugin_line(const std::string& raw, const char* path, bool& enabled) {
    const std::string line = trim(raw);
    if (line.empty() || line[0] == ';' || line[0] == '#') return false;
    const std::size_t eq = line.find('=');
    const std::string configured = trim(line.substr(0, eq));
    if (configured != path) return false;
    enabled = eq == std::string::npos || trim(line.substr(eq + 1)) != "false";
    return true;
}

bool saved_state(const char* path) {
    std::vector<std::string> lines;
    if (!read_lines(lines)) return false;
    bool in_default = false;
    bool enabled = false;
    for (const auto& raw : lines) {
        const std::string line = trim(raw);
        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            in_default = line == "[default]";
            continue;
        }
        if (in_default && parse_plugin_line(raw, path, enabled)) return enabled;
    }
    return false;
}

bool backup_config() {
    struct stat info{};
    if (::stat(GHEN_BACKUP, &info) == 0) return true;
    FILE* source = std::fopen(GHEN_CONFIG, "rb");
    if (!source) return true;
    FILE* backup = std::fopen(GHEN_BACKUP, "wb");
    if (!backup) { std::fclose(source); return false; }
    char buffer[4096];
    std::size_t count = 0;
    bool ok = true;
    while ((count = std::fread(buffer, 1, sizeof(buffer), source)) != 0) {
        if (std::fwrite(buffer, 1, count, backup) != count) { ok = false; break; }
    }
    if (std::ferror(source)) ok = false;
    if (std::fclose(source) != 0) ok = false;
    if (std::fclose(backup) != 0) ok = false;
    if (!ok) std::remove(GHEN_BACKUP);
    return ok;
}

bool write_goldhen_config(const PluginEntry* entries, std::size_t count) {
    std::vector<std::string> lines;
    if (!read_lines(lines) || !backup_config()) return false;

    std::vector<std::string> updated;
    updated.reserve(lines.size() + count + 2);
    bool in_default = false;
    bool default_found = false;
    bool inserted = false;

    for (const auto& raw : lines) {
        const std::string line = trim(raw);
        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            if (in_default && !inserted) {
                for (std::size_t i = 0; i < count; ++i)
                    updated.emplace_back(std::string(entries[i].path) +
                        (entries[i].enabled ? "=true\n" : "=false\n"));
                inserted = true;
            }
            in_default = line == "[default]";
            if (in_default) default_found = true;
            updated.push_back(raw);
            continue;
        }
        if (in_default) {
            bool is_managed = false;
            for (std::size_t i = 0; i < count; ++i) {
                bool ignored = false;
                if (parse_plugin_line(raw, entries[i].path, ignored)) {
                    is_managed = true;
                    break;
                }
            }
            if (is_managed) continue;
        }
        updated.push_back(raw);
    }

    if (in_default && !inserted) {
        for (std::size_t i = 0; i < count; ++i)
            updated.emplace_back(std::string(entries[i].path) +
                (entries[i].enabled ? "=true\n" : "=false\n"));
        inserted = true;
    }
    if (!default_found) {
        if (!updated.empty() && updated.back().find('\n') == std::string::npos)
            updated.back().push_back('\n');
        updated.emplace_back("\n[default]\n");
        for (std::size_t i = 0; i < count; ++i)
            updated.emplace_back(std::string(entries[i].path) +
                (entries[i].enabled ? "=true\n" : "=false\n"));
    }

    FILE* output = std::fopen(GHEN_TEMP, "w");
    if (!output) return false;
    bool ok = true;
    for (const auto& line : updated) {
        if (std::fwrite(line.data(), 1, line.size(), output) != line.size()) {
            ok = false;
            break;
        }
    }
    if (std::fclose(output) != 0) ok = false;
    if (!ok || std::rename(GHEN_TEMP, GHEN_CONFIG) != 0) {
        std::remove(GHEN_TEMP);
        return false;
    }
    return true;
}
} // namespace

std::size_t refresh_plugins(PluginEntry* entries, std::size_t capacity) {
    if (!entries || capacity == 0 || !ensure_directories()) return 0;
    DIR* dir = ::opendir(PLUGIN_DIR);
    if (!dir) return 0;

    std::size_t count = 0;
    dirent* item = nullptr;
    while ((item = ::readdir(dir)) != nullptr && count < capacity) {
        if (!ends_with_sprx(item->d_name)) continue;
        struct stat info{};
        char full_path[256]{};
        const int written = std::snprintf(full_path, sizeof(full_path),
                                          "%s/%s", PLUGIN_DIR, item->d_name);
        if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(full_path))
            continue;
        if (::stat(full_path, &info) != 0 || !S_ISREG(info.st_mode)) continue;

        PluginEntry& entry = entries[count];
        std::memset(&entry, 0, sizeof(entry));
        std::strncpy(entry.filename, item->d_name, sizeof(entry.filename) - 1);
        const std::size_t name_len = std::strlen(item->d_name) - 5;
        const std::size_t copy_len = name_len < sizeof(entry.name) - 1
            ? name_len : sizeof(entry.name) - 1;
        for (std::size_t i = 0; i < copy_len; ++i) {
            entry.name[i] = static_cast<char>(
                std::toupper(static_cast<unsigned char>(item->d_name[i])));
        }
        std::strncpy(entry.path, full_path, sizeof(entry.path) - 1);
        entry.size = static_cast<std::uint64_t>(info.st_size);
        entry.enabled = saved_state(entry.path);
        ++count;
    }
    ::closedir(dir);
    return count;
}

bool set_plugin_enabled(PluginEntry* entries, std::size_t count,
                        std::size_t index, bool enabled) {
    if (!entries || index >= count) return false;
    const bool previous = entries[index].enabled;
    entries[index].enabled = enabled;
    if (write_goldhen_config(entries, count)) return true;
    entries[index].enabled = previous;
    return false;
}

const char* plugin_directory() { return PLUGIN_DIR; }
bool plugin_storage_ready() { return ensure_directories(); }

} // namespace vypex
