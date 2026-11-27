#include "vypex/core.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace vypex;
    Config config = Config::defaults();
    assert(!config.block_updates && config.show_notifications && !config.plugin_autoload);

    Config parsed{};
    assert(Config::parse("block_updates=1\nshow_notifications=0\nplugin_autoload=1\n", parsed));
    assert(parsed.block_updates && !parsed.show_notifications && parsed.plugin_autoload);
    assert(!Config::parse("block_updates=maybe\n", parsed));

    PluginRegistry registry;
    assert(registry.add({"Overlay", "data/overlay.prx", true}));
    assert(!registry.add({"Overlay", "other.prx", true}));
    assert(registry.count() == 1);
    assert(registry.find("Overlay") != nullptr);
    assert(registry.set_enabled("Overlay", false));
    assert(!registry.find("Overlay")->enabled);
    assert(!registry.set_enabled("Missing", true));
    return 0;
}
