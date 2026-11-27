#include "ps4.h"

#include <stdint.h>

#define FW_TARGET 1402
#define USER_ID_FALLBACK 0x10000000

#define PAD_UP      0x00000010u
#define PAD_RIGHT   0x00000020u
#define PAD_DOWN    0x00000040u
#define PAD_LEFT    0x00000080u
#define PAD_CROSS   0x00004000u
#define PAD_CIRCLE  0x00002000u

typedef struct {
    uint32_t buttons;
    uint8_t rest[116];
} PlutanaPadData;

static void show_menu(int selected) {
    static const char *items[] = {
        "> SYSTEM STATUS\n  ABOUT PLUTANA-HEN\n  EXIT",
        "  SYSTEM STATUS\n> ABOUT PLUTANA-HEN\n  EXIT",
        "  SYSTEM STATUS\n  ABOUT PLUTANA-HEN\n> EXIT"
    };
    printf_notification("PLUTANA-HEN  /  FW 14.02\n\n%s\n\nUP/DOWN SELECT  CROSS OPEN  CIRCLE BACK", items[selected]);
}

static void show_status(void) {
    const int root = (getuid() == 0);
    const int sandboxed = is_in_sandbox();
    printf_notification("PLUTANA-HEN STATUS\nFirmware: 14.02\nUID: %d\nSandbox: %s\nPayload: ACTIVE",
                        getuid(), sandboxed == 0 ? "OFF" : "ON");
    (void)root;
}

int _main(void) {
    initKernel();
    initLibc();

    const uint16_t fw = get_firmware();
    initSysUtil();

    if (fw != FW_TARGET) {
        printf_notification("PLUTANA-HEN\nUnsupported firmware profile: %u\nThis build is for 14.02 only.", fw);
        return 0;
    }

    if (!is_jailbroken()) {
        printf_notification("PLUTANA-HEN\nPayload loaded, but no privileged session is active.\nThis build does not include a firmware exploit.");
        return 0;
    }

    initPad();
    int user = getInitialUser();
    if (user < 0) user = USER_ID_FALLBACK;

    const int pad = scePadOpen(user, 0, 0, NULL);
    if (pad < 0) {
        printf_notification("PLUTANA-HEN\nPayload initialized on FW 14.02.\nController unavailable; no menu input handle could be opened.");
        return 0;
    }

    printf_notification("PLUTANA-HEN\n14.02 payload initialized.\nController menu ready.");
    int selected = 0;
    uint32_t previous = 0;
    PlutanaPadData data = {0};
    show_menu(selected);

    for (;;) {
        if (scePadReadState(pad, &data) == 0) {
            const uint32_t buttons = data.buttons;
            const uint32_t pressed = buttons & ~previous;
            previous = buttons;

            if (pressed & PAD_UP) {
                selected = (selected + 2) % 3;
                show_menu(selected);
            } else if (pressed & PAD_DOWN) {
                selected = (selected + 1) % 3;
                show_menu(selected);
            } else if (pressed & PAD_CROSS) {
                if (selected == 0) {
                    show_status();
                    show_menu(selected);
                } else if (selected == 1) {
                    printf_notification("ABOUT PLUTANA-HEN\nStandalone PS4 payload\nFirmware target: 14.02\nNo GoldHEN plugin dependency.");
                    show_menu(selected);
                } else {
                    break;
                }
            } else if (pressed & PAD_CIRCLE) {
                break;
            }
        }
        sceKernelUsleep(16667);
    }

    scePadClose(pad);
    printf_notification("PLUTANA-HEN\nMenu closed. Payload session ending.");
    return 0;
}
