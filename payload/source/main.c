#include "ps4.h"
#include "services.h"

#include <stdint.h>

#define FW_TARGET 1402

/*
 * Plutana-HEN resident payload entry point.
 *
 * This payload is intended to be loaded by an existing PS4 payload loader.
 * It does not exploit the console or attempt to unload/replace another
 * resident payload. Controller PS-button events are not exposed by the
 * documented libScePad button-state API, so no PS-button hot-switch is
 * advertised as working here.
 */
int _main(void) {
    initKernel();
    initLibc();
    initSysUtil();

    const uint16_t firmware = get_firmware();
    if (firmware != FW_TARGET) {
        printf_notification("Plutana-HEN",
            "Plutana-HEN not loaded: this build requires PS4 firmware 14.02.");
        return 0;
    }

    initNetwork();
    initPthread();

    if (plutana_services_start() != 0) {
        printf_notification("Plutana-HEN",
            "Plutana-HEN startup failed. Check /data/Plutana/plutana.log.");
        return -1;
    }

    plutana_log("INFO", "Plutana-HEN loaded successfully on firmware 14.02.");
    printf_notification("Plutana-HEN",
        "Plutana-HEN loaded. FTP:2121 | Logs:3232.");

    /*
     * The PS button is reserved by the PS4 system shell and is absent from
     * the documented scePadReadState button mask. A five-PS-button-press
     * GoldHEN handoff cannot safely be implemented through that API. Do not
     * claim that pressing PS five times switches payloads.
     */
    printf_notification("Plutana-HEN",
        "PS-button x5 switching is not available in this build.");

    return 0;
}
