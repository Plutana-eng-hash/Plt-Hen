#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vypex {

struct Config {
    bool block_updates;
    bool show_notifications;
    bool plugin_autoload;

    static Config defaults();
    std::string serialize() const;
    static bool parse(const std::string& text, Config& out);
};

struct PluginRecord {
    std::string name;
    std::string path;
    bool enabled;
};

class PluginRegistry {
public:
    bool add(const PluginRecord& record);
    bool set_enabled(const std::string& name, bool enabled);
    const PluginRecord* find(const std::string& name) const;
    std::size_t count() const;
    const std::vector<PluginRecord>& entries() const;

private:
    std::vector<PluginRecord> entries_;
};

const char* runtime_name();

} // namespace vypex
