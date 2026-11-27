# Vypex-Cr_ck — native PS4 homebrew foundation

Vypex-Cr_ck is an original native PS4 homebrew codebase targeting firmware 14.02. It is **not** a browser application and is not a GoldHEN fork or rebrand.

## Current build scope

- Native C++ project built for the PS4 target with the OpenOrbis toolchain.
- Purple Vypex identity and a small native runtime/status core.
- Plugin registry model (metadata, enable/disable state, validation and listing).
- Configuration parsing/serialization with safe defaults.
- GitHub Actions builds a PS4-target ELF artifact and runs host-side tests.

## Important limitations

This repository does not currently contain a kernel exploit, firmware-specific kernel offsets, kernel patches, privilege escalation, or a PS4 system-shell replacement. Therefore it is **not a jailbreak/HEN** and cannot enable PRX loading or privileged plugin management by itself. Those features require a separately supplied, compatible privileged runtime. The build artifact is a native homebrew ELF, not an installable PKG.

## Build

Push to `main` or run **Actions → Build Vypex-Cr_ck native**. Download the `Vypex-Cr_ck-PS4-ELF` artifact from the run.

The workflow installs the OpenOrbis PS4 toolchain, compiles the target, and runs the host-side core tests.
