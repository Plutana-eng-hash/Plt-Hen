#include "ps4.h"
#include "services.h"

#include <stdint.h>

#define FW_TARGET 1402

/*
 * Plutana-HEN resident payload entry point.
 *
 * This build deliberately does not run an exploit or apply unverified kernel
 * patches. It starts independent userland services after the existing loader
 * has transferred control to this payload.
 */
int _main(void) {
    initKernel();
    initLibc();
    initSysUtil();

    const uint16_t firmware = get_firmware();
    if (firmware != FW_TARGET) {
        printf_notification("PLUTANA-HEN",
            "Unsupported firmware. This build is for PS4 14.02 only.");
        return 0;
    }

    initNetwork();
    initPthread();

    if (plutana_services_start() != 0) {
        printf_notification("PLUTANA-HEN",
            "Service startup failed. Check /data/Plutana/plutana.log.");
        return -1;
    }

    printf_notification("PLUTANA-HEN",
        "Loaded. FTP:2121  Klog:3232. See /data/Plutana/plutana.log.");
    return 0;
}
