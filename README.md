# Vypex-Cr_ck — native PS4 homebrew + GoldHEN plugin

Vypex-Cr_ck targets PS4 firmware 14.02 and has two separate components:

- **Vypex-Cr_ck.elf** — the native controller-driven configuration UI.
- **Vypex-Cr_ck.prx** — a GoldHEN-loadable user-mode plugin with load/unload callbacks and a small per-process service-control worker.

This is not a jailbreak, HEN replacement, kernel patch, or PS4 shell replacement.

## GoldHEN PRX behavior

The PRX uses the public GoldHEN plugin lifecycle (`plugin_load` / `plugin_unload`) and standard module lifecycle entry points. GoldHEN's plugin loader injects configured plugins into application processes, so the PRX is instantiated per process when a title/application starts; it is not one global daemon that exists before a title launches.

The PRX starts a lightweight worker in that process. It watches `/data/Vypex-Cr_ck/service.enabled` and updates `/data/Vypex-Cr_ck/service.status`. The Vypex UI's **Settings → Vypex Background Service** toggle writes `1` or `0` to that control file. Turning it off pauses the Vypex service behavior; the small control worker remains until GoldHEN calls `plugin_unload` or the host process exits. This toggle does **not** forcibly unmap the PRX from a running process.

The PRX's `plugin_unload` callback stops and joins its worker. GoldHEN loads configured plugins when an application/game process starts, so changes to the plugin list apply to subsequently launched processes. GoldHEN Plugin Loader must be enabled in GoldHEN settings.

## Install

1. Load GoldHEN 14.02 and enable its **Plugin Loader** setting.
2. Copy `Vypex-Cr_ck.prx` to `/data/Vypex-Cr_ck/plugins/Vypex-Cr_ck.prx` on the PS4.
3. Launch the Vypex ELF UI and open **Plugins** so it can detect `/data/Vypex-Cr_ck/plugins/Vypex-Cr_ck.prx`. On detection, Vypex adds `/data/Vypex-Cr_ck/plugins/Vypex-Cr_ck.prx=true` to GoldHEN's `[default]` section if no entry already exists. An existing explicit `=false` choice is respected.
4. Close and relaunch an application/game so GoldHEN loads the configured PRX into that process.
5. Open Vypex **Settings** to pause/resume the Vypex background service.

Vypex backs up the original GoldHEN plugin configuration to `/data/GoldHEN/plugins.ini.vypex.bak` before its first modification and preserves `[settings]` and unrelated title-ID sections. The `[default]` entry makes GoldHEN load the PRX for every subsequently launched title; GoldHEN Plugin Loader must be enabled, and the PRX must already be copied to the exact path above.

## Build

GitHub Actions builds both outputs. The PRX job downloads the public [GoldHEN Plugins SDK](https://github.com/GoldHEN/GoldHEN_Plugins_SDK) and OpenOrbis toolchain. It produces:

- `build/Vypex-Cr_ck.elf`
- `build-prx/out/Vypex-Cr_ck.prx`

Run **Actions → Build Vypex-Cr_ck native and GoldHEN PRX** and download the combined artifact after the run succeeds.

## Limitations

- The PRX is a normal GoldHEN plugin, not a new jailbreak or kernel payload.
- It does not survive a full power-off/reboot without GoldHEN being activated again.
- The service toggle pauses Vypex's worker behavior; it is not a forced self-unload of the mapped PRX.
- GoldHEN injects plugins per application process. It is not a global system service.
- The native ELF UI and PRX are separate binaries; the PRX does not display the framebuffer UI over the PS4 shell or games.
- Controller compatibility is limited to devices exposed by the PS4 through the standard `scePad` API.
