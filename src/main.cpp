#include "vypex/core.hpp"

#include <cstdio>

int main() {
    vypex::Config config = vypex::Config::defaults();
    vypex::PluginRegistry plugins;

    std::puts("");
    std::puts("  V Y P E X - C r _ c k");
    std::puts("  Native PS4 homebrew foundation");
    std::puts("  Firmware profile: 14.02 (configured target)");
    std::puts("");
    std::printf("  Runtime: %s\n", vypex::runtime_name());
    std::printf("  Plugin records: %zu\n", plugins.count());
    std::printf("  Update blocking preference: %s\n",
                config.block_updates ? "enabled" : "disabled");
    std::puts("");
    std::puts("  This build is a user-mode homebrew foundation.");
    std::puts("  It does not jailbreak or patch the PS4 kernel.");
    std::puts("");
    return 0;
}
