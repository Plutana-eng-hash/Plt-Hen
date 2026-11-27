#include "ps4.h"

#include <stdint.h>

#define FW_TARGET 1402

/*
 * Raw payload bootstrap only.
 *
 * The actual interactive framebuffer UI is the native OpenOrbis application
 * in src/main.cpp. A raw payload executes in a different runtime and cannot
 * reuse that application's VideoOut framebuffer/session. Keep this entry
 * point free of a fake notification-based "menu".
 */
int _main(void) {
    initKernel();
    initLibc();
    initSysUtil();

    const uint16_t fw = get_firmware();
    if (fw != FW_TARGET) {
        printf_notification("PLUTANA-HEN", "Unsupported firmware. This payload targets PS4 firmware 14.02 only.");
        return 0;
    }

    if (!is_jailbroken()) {
        printf_notification("PLUTANA-HEN", "Payload loaded, but no privileged session is active. This build does not contain a jailbreak exploit.");
        return 0;
    }

    printf_notification("PLUTANA-HEN", "14.02 payload initialized. Launch the Plutana-HEN native app for the full controller-driven graphical menu.");
    return 0;
}
