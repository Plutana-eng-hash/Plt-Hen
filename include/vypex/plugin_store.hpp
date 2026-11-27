#pragma once

#include <cstddef>
#include <cstdint>

namespace vypex {

struct PluginEntry {
    char name[64];
    char filename[128];
    char path[256];
    std::uint64_t size;
    bool enabled;
};

// Scans the PS4 data partition for .prx files and restores saved enable flags.
// This is discovery/configuration only; it does not execute PRX files.
std::size_t refresh_plugins(PluginEntry* entries, std::size_t capacity);
bool set_plugin_enabled(PluginEntry* entries, std::size_t count,
                        std::size_t index, bool enabled);
const char* plugin_directory();
bool plugin_storage_ready();

} // namespace vypex
