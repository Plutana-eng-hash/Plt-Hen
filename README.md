# Vypex-Cr_ck — native PS4 homebrew foundation

Vypex-Cr_ck is an original native PS4 homebrew codebase targeting firmware 14.02. It is not a browser application, a GoldHEN fork, or a jailbreak implementation.

## Current build scope

- Native C++ PS4 ELF built with the OpenOrbis toolchain.
- Purple Vypex framebuffer interface and controller navigation.
- PS4 controller input through the standard `scePad` interface.
- Local discovery of `.sprx` files in `/data/Vypex-Cr_ck/plugins`.
- Plugin toggles update GoldHEN's `/data/GoldHEN/plugins.ini` **[default]** section, preserving unrelated sections and creating a one-time backup at `plugins.ini.vypex.bak`.
- GoldHEN plugin configuration is read back by Vypex so the UI reflects the configured enabled/disabled state.
- Atomic temporary-file replacement is used when writing the GoldHEN plugin configuration.
- Host-side configuration tests and a GitHub Actions PS4 ELF build.

## GoldHEN 14.02 integration

1. Load GoldHEN first and make sure its **Plugin Loader** option is enabled.
2. Copy compatible, firmware-appropriate `.sprx` plugins into `/data/Vypex-Cr_ck/plugins/`.
3. Start Vypex, open **Plugins**, and toggle entries with Cross.
4. Vypex writes those entries into the **[default]** section of `/data/GoldHEN/plugins.ini`. Existing entries in other title-ID sections are retained.
5. GoldHEN loads enabled entries when a title/application process starts. A configuration change does not retroactively load or unload a plugin already mapped into a running process; close and relaunch the affected title to apply it.

Vypex currently manages GoldHEN's plugin configuration. It is **not itself a GoldHEN PRX**, does not start a resident Vypex daemon, and cannot unload a PRX already running in another process. The app's exit control only exits the Vypex UI. A true in-session Vypex service with a self-unload command requires a separately built and tested PRX using the GoldHEN plugin SDK and a defined service lifecycle; this repository does not claim that exists yet.

## Controller compatibility

The app reads controllers exposed by the PS4 through `scePad`. It does not install USB/Bluetooth drivers or convert an unsupported controller into a system-wide DualShock 4. Xbox and DualSense controllers require a PS4-compatible adapter/translator if the console does not already recognize them.

## Important limitations

This repository does not contain a kernel exploit, firmware-specific kernel offsets, kernel patches, privilege escalation, or a PS4 system-shell replacement. It is a native user-mode homebrew app, not a jailbreak/HEN. The build artifact is an ELF, not an installable PKG.

## Build

Push to `main` or run **Actions → Build Vypex-Cr_ck native**. Download the `Vypex-Cr_ck-PS4-ELF` artifact from a successful workflow run.
