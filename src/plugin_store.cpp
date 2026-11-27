#include "vypex/plugin_store.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace vypex {
namespace {
constexpr const char* ROOT = "/data/Vypex-Cr_ck";
constexpr const char* PLUGIN_DIR = "/data/Vypex-Cr_ck/plugins";
constexpr const char* STATE_FILE = "/data/Vypex-Cr_ck/plugins.ini";
constexpr const char* TEMP_FILE = "/data/Vypex-Cr_ck/plugins.ini.tmp";

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

bool saved_state(const char* filename) {
    FILE* file = std::fopen(STATE_FILE, "r");
    if (!file) return false;
    char line[192];
    bool enabled = false;
    while (std::fgets(line, sizeof(line), file)) {
        char* equal = std::strchr(line, '=');
        if (!equal) continue;
        *equal++ = '\0';
        if (std::strcmp(line, filename) == 0) {
            enabled = (*equal == '1');
            break;
        }
    }
    std::fclose(file);
    return enabled;
}

bool save_states(const PluginEntry* entries, std::size_t count) {
    FILE* file = std::fopen(TEMP_FILE, "w");
    if (!file) return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (std::fprintf(file, "%s=%d\n", entries[i].filename,
                         entries[i].enabled ? 1 : 0) < 0) {
            std::fclose(file);
            std::remove(TEMP_FILE);
            return false;
        }
    }
    if (std::fclose(file) != 0) {
        std::remove(TEMP_FILE);
        return false;
    }
    if (std::rename(TEMP_FILE, STATE_FILE) != 0) {
        std::remove(TEMP_FILE);
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
        entry.enabled = saved_state(entry.filename);
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
    if (save_states(entries, count)) return true;
    entries[index].enabled = previous;
    return false;
}

const char* plugin_directory() { return PLUGIN_DIR; }
bool plugin_storage_ready() { return ensure_directories(); }

} // namespace vypex
