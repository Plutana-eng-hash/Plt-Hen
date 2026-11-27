# Plutana-HEN raw payload target (PS4 14.02)

This directory builds a raw PS4 payload binary using the 14.02-enabled
[iHaiDeeZ PS4 Payload SDK](https://github.com/iHaiDeeZ/ps4-payload-sdk).

## Output

`Plutana-HEN.bin` is emitted by the SDK linker script and `objcopy -O binary`.
It is a raw payload, not an ELF, PRX, EBOOT, or PKG.

## Responsibility split

- `Plutana-HEN.bin` is a small payload-stage bootstrap. It verifies firmware
  14.02 and checks that a privileged session is already active.
- The actual controller-driven graphical menu is the native OpenOrbis app
  built from `src/main.cpp`. It draws through the PS4 VideoOut framebuffer,
  supports D-pad/Cross/Circle navigation, and contains the Games, Homebrew,
  Packages, Controllers, Plugins, Settings, and About sections.
- The graphical app is a separately launched user-mode application. The raw
  payload does not pretend to be a shell overlay and does not show a fake
  notification-based menu.

## Security and compatibility limits

This payload does not contain a firmware exploit or kernel patch chain. It
cannot jailbreak a stock 14.02 console on its own. The SDK's generic jailbreak
helper does not supply a complete 14.02 exploit implementation.

The native menu is not a replacement for the PS4 system shell and does not
draw over games. A shell/game overlay would require a separate, verified
graphics hook/runtime. Do not rename an ELF or PKG to `.bin`; they are
different executable formats.

## Build

GitHub Actions builds the native ELF, GoldHEN PRX, and raw payload as separate
artifacts. The payload Makefile provides the default `all` target expected by
the SDK's Docker build action.
