# Plutana-HEN

Plutana-HEN is a reproducible build wrapper for the public PS4-HEN source tree, pinned to the upstream commit that added firmware **14.02** support. The repository builds a raw `hen.bin` payload through GitHub Actions and stores build outputs as workflow artifacts.

> **Status:** Build integration is being established. A successful CI compile does not establish that the payload works on a physical PS4. Hardware testing on a 14.02 console is still required.

## Source components

| Component | Pinned source |
|---|---|
| HEN payload | [Scene-Collective/ps4-hen](https://github.com/Scene-Collective/ps4-hen), commit `8c62f6628827055df0dfece1c4d14e045761288b` |
| PS4 payload SDK | [Scene-Collective/ps4-payload-sdk](https://github.com/Scene-Collective/ps4-payload-sdk), commit `fd4f7299d1da5f5601c164a02b6d6d019ac8630a` |

The HEN source is a Git submodule under `vendor/ps4-hen`. Clone this project with submodules:

```bash
git clone --recurse-submodules https://github.com/Plutana-eng-hash/Plt-Hen.git
cd Plt-Hen
```

## Build

On Ubuntu 22.04+ or WSL2 Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential binutils gcc make curl unzip xxd git
./scripts/build-payload.sh
```

The script checks the pinned upstream revision, builds the SDK's `libPS4`, runs the upstream HEN build, checks that the result is a non-empty raw payload, and writes:

- `out/Plutana-HEN-14.02.bin`
- `out/Plutana-HEN-14.02.bin.sha256`
- `out/build-info.txt`

The first build downloads the upstream HEN plugin bundle as required by its upstream build script. That bundle is currently obtained from the upstream project's latest plugin release; it is therefore a separately moving input even though the HEN and SDK source commits are pinned.

## GitHub Actions

Pushes to `main` and manual workflow dispatch run the build. The workflow uploads the files from `out/` as a downloadable Actions artifact. It does not publish a GitHub Release automatically.

## Firmware and loader notes

The pinned HEN source commit adds the 14.02 firmware offset table, registers firmware 1402 in the firmware selector, and raises the maximum supported firmware constant to 1402. The upstream README has not yet caught up with that commit, so this repository records the exact source revision instead of relying on the stale README version list.

This repository builds the **HEN payload only**. It is not a browser exploit or an exploit host. Payload execution requires a separately obtained, compatible payload loader. The separate [Afterglow web host](https://github.com/well0nez/afterglow-web) documents 14.02 support; it is not copied into this repository.

## What is and is not verified

- Source revision contains the 14.02 offset implementation: verified from the upstream commit.
- Build pipeline and generated binary: to be confirmed by the first GitHub Actions run.
- Runtime stability and all HEN features on retail 14.02 hardware: not verified by this repository's CI.

Use only on hardware you own and keep backups of important data. Kernel-level modifications can crash or require a full reboot.

## Upstream ownership and licensing

The PS4-HEN and PS4 payload SDK sources remain separate upstream projects and retain their own notices and terms. This repository does not claim authorship of, or relicense, those dependencies. Only the build-wrapper files authored here are Plutana-HEN project files.
