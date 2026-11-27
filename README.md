# Plutana-HEN — single-file PS4 payload

This branch publishes **one installable build artifact: `Plutana-HEN.bin`**.

## Build output

GitHub Actions runs the host tests and builds the raw PS4 14.02 payload. The only downloadable build artifact is:

- `Plutana-HEN.bin` — raw payload for an existing PS4 payload loader.

Open **Actions → Build Plt-Hen native, PRX, and payload** and download the `Plutana-HEN-ONE-BIN` artifact.

## Important scope

This single BIN is the existing resident payload. It starts Plutana's background services and reports its load status. It does **not** contain the separate native full-screen UI, does not replace Sony's PS4 Home/ShellUI, and does not implement PS-button switching. Those components currently require separate ELF/PRX build targets and cannot be made into a working single raw payload merely by concatenating their files.

A true one-BIN Home replacement requires porting and validating the UI and supported ShellUI integration into one compatible payload runtime for firmware 14.02. This build does not claim that work is complete.

## Firmware

The payload checks for PS4 firmware 14.02 and refuses to start on other firmware versions.
