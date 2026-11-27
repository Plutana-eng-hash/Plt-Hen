# Vypex-Cr_ck — Control Center

A purple-first, responsive homebrew workspace interface with its own visual identity. It is an independent interface project, not a GoldHEN fork or rebrand.

## Included
- Responsive dark/purple dashboard and navigation
- Plugin-entry library with add, edit, remove, and search
- Browser-local preferences and persistence
- Firmware target label (informational only)
- Accessible form labels, keyboard-friendly controls, and status feedback

## Run locally
Open `index.html` in a modern browser. No build dependencies are required.

## Build
The GitHub Actions workflow validates the JavaScript and required assets, then packages:
`Vypex-Cr_ck-Interface.zip`

Download it from the **Actions → Build Vypex-Cr_ck interface → Artifacts** section.

## Important scope
This repository currently builds the Vypex-Cr_ck interface only. It does **not** include a PS4 jailbreak, kernel exploit, firmware offsets, kernel patches, a PRX loader, or native PS4 system-menu integration. The plugin library stores local reference entries in the browser; it does not install or load plugins on a console. The displayed firmware target is a label, not live hardware detection.

The project is not affiliated with GoldHEN. It uses an original Vypex visual design rather than copying GoldHEN assets or claiming its functionality.
