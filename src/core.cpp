#include "vypex/core.hpp"

#include <algorithm>
#include <sstream>

namespace vypex {

Config Config::defaults() {
    return {false, true, false};
}

std::string Config::serialize() const {
    std::ostringstream out;
    out << "block_updates=" << (block_updates ? 1 : 0) << '\n'
        << "show_notifications=" << (show_notifications ? 1 : 0) << '\n'
        << "plugin_autoload=" << (plugin_autoload ? 1 : 0) << '\n';
    return out.str();
}

bool Config::parse(const std::string& text, Config& out) {
    Config candidate = defaults();
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        const auto split = line.find('=');
        if (split == std::string::npos || split == 0 || split + 2 != line.size())
            return false;
        const std::string key = line.substr(0, split);
        const char value = line[split + 1];
        if (value != '0' && value != '1') return false;
        const bool enabled = value == '1';
        if (key == "block_updates") candidate.block_updates = enabled;
        else if (key == "show_notifications") candidate.show_notifications = enabled;
        else if (key == "plugin_autoload") candidate.plugin_autoload = enabled;
        else return false;
    }
    out = candidate;
    return true;
}

bool PluginRegistry::add(const PluginRecord& record) {
    if (record.name.empty() || record.path.empty() || find(record.name)) return false;
    if (record.name.size() > 48 || record.path.size() > 240) return false;
    entries_.push_back(record);
    return true;
}

bool PluginRegistry::set_enabled(const std::string& name, bool enabled) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [&](const PluginRecord& item) { return item.name == name; });
    if (it == entries_.end()) return false;
    it->enabled = enabled;
    return true;
}

const PluginRecord* PluginRegistry::find(const std::string& name) const {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [&](const PluginRecord& item) { return item.name == name; });
    return it == entries_.end() ? nullptr : &*it;
}

std::size_t PluginRegistry::count() const { return entries_.size(); }
const std::vector<PluginRecord>& PluginRegistry::entries() const { return entries_; }

const char* runtime_name() { return "Vypex native core"; }

} // namespace vypex
