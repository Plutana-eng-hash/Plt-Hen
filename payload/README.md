# Plutana-HEN raw payload — PS4 firmware 14.02

This directory builds `Plutana-HEN.bin`, a raw payload for an already-enabled
PS4 environment. It is not an ELF, PRX, EBOOT, or PKG.

## Implemented in this build

- Resident startup: starts detached service threads and returns from the
  payload entry point without ending those workers.
- FTP server on TCP port 2121:
  - Login: `plutana`
  - Password: `Plutana14.02!`
  - Passive mode, directory listing, download, upload, SIZE, CWD, PWD.
  - FTP is deliberately rooted at `/data/Plutana`; it cannot traverse to
    arbitrary console paths.
- Userland log stream on TCP port 3232. Connect with a TCP client to receive
  the current log snapshot.
- Runtime log file: `/data/Plutana/plutana.log`.

## Not implemented / not claimed

This is not a complete GoldHEN replacement yet. GoldHEN's published feature
list includes kernel and system patches, remote package installation,
plugin loading, rest-mode handling, overlays, and other features. This
payload currently implements the userland FTP and log services only.

The public PS4 payload SDK has a 14.02 XFAST_SYSCALL offset, but its 14.02
PRISON_0 value is explicitly marked as a placeholder and its ROOTVNODE
support is not verified. The SDK's generic jailbreak helper therefore
returns without applying the 14.02 credential/root patches. Plutana does not
write kernel memory using those unverified values.

The BinLoader on port 9090 is not enabled in this build. Executing arbitrary
network-delivered payloads requires a separately verified executable-memory
and thread-entry implementation; a TCP listener that merely accepts bytes
would not be a working BinLoader.

## Build

GitHub Actions uses the Scene Collective PS4 Payload SDK fork with 14.02
support. The output is checked as a non-empty raw binary. A successful build
does not by itself prove runtime compatibility on physical hardware.

## FTP security note

Change the default FTP password in `source/services.c` before using this
service on an untrusted network. The FTP root is limited to
`/data/Plutana`, but the service still permits file uploads and downloads
within that directory.
