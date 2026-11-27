# Plutana-HEN payload target (PS4 14.02)

This directory builds a raw PS4 payload binary using the 14.02-enabled
[iHaiDeeZ PS4 Payload SDK](https://github.com/iHaiDeeZ/ps4-payload-sdk).

## Output

`Plutana-HEN.bin` is emitted by the SDK linker script and `objcopy -O binary`.
It is a raw payload, not an ELF, PRX, EBOOT, or PKG.

## Current payload behavior

- Verifies that the running console reports firmware 14.02.
- Requires an already privileged / jailbroken runtime; it does not contain a
  firmware exploit or kernel patch chain.
- Loads the PS4 controller module and provides a small notification-based
  controller menu (D-pad, Cross, Circle).
- Displays status and build information through PS4 system notifications.

This is deliberately separate from GoldHEN's plugin ABI. The payload SDK's
14.02 fork currently leaves its generic `jailbreak()` helper as a no-op for
14.02 because the required prison/rootvnode offsets are not resolved there.
Therefore this payload does not claim to jailbreak a stock or merely exploited
14.02 console by itself.

The notification menu is a first payload-stage interface, not the full
framebuffer overlay from the existing ELF application. The SDK does not expose
the OpenOrbis VideoOut framebuffer APIs used by that ELF. A true shell/game
overlay needs a separate, verified graphics hook/runtime and must not be
faked by relabeling the ELF.

## Build

The repository GitHub Actions workflow invokes the SDK's Docker-based build
action and uploads `payload/Plutana-HEN.bin` as a separate artifact.
