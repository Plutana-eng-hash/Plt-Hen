# Vypex-Cr_ck — native PS4 homebrew foundation

Vypex-Cr_ck is an original native PS4 homebrew codebase targeting firmware 14.02. It is **not** a browser application and is not a GoldHEN fork or rebrand.

## Current build scope

- Native C++ project built for the PS4 target with the OpenOrbis toolchain.
- Purple Vypex identity and native framebuffer interface.
- Controller input through the PS4 `scePad` API, polling up to four standard controller slots.
- Controller disconnect handling and periodic reconnection attempts.
- Controller status page in the interface.
- Plugin registry model (metadata, enable/disable state, validation and listing).
- Configuration parsing/serialization with safe defaults.
- GitHub Actions builds a PS4-target ELF artifact and runs host-side tests.

## Controller compatibility

The application reads controllers that the PS4 operating system exposes through its standard `scePad` interface. This is not a universal USB/Bluetooth HID driver and it cannot pair unsupported controllers or impersonate a DualShock 4 at the console/system level.

- DualShock 4 and other devices already recognized by the PS4 can be read by the app.
- Xbox controllers and DualSense controllers are **not made PS4-compatible by this application**. Use a PS4-compatible hardware adapter/translator if you want to use one on the console; the adapter must present itself to the PS4 as a supported controller.
- The controller status page reports the number of active standard controller slots. It does not identify every controller model or guarantee compatibility with every third-party device.
- Multi-controller polling is implemented in source, but must still be tested on a physical PS4 and with each specific controller/adapter.

## Important limitations

This repository does not currently contain a kernel exploit, firmware-specific kernel offsets, kernel patches, privilege escalation, or a PS4 system-shell replacement. Therefore it is **not a jailbreak/HEN** and cannot enable PRX loading or privileged plugin management by itself. Those features require a separately supplied, compatible privileged runtime. The build artifact is a native homebrew ELF, not an installable PKG.

## Build

Push to `main` or run **Actions → Build Vypex-Cr_ck native**. Download the `Vypex-Cr_ck-PS4-ELF` artifact from the run.

The workflow installs the OpenOrbis PS4 toolchain, compiles the target, and runs the host-side core tests.
