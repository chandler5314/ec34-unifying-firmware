#include "ec34u_radio_esb_zephyr.h"
#include "ec34u_unifying.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void) {
    struct ec34u_profile profile;
    struct ec34u_pairing_state state;
    struct ec34u_radio_ops radio;
    uint8_t rf_address[EC34U_RF_ADDR_LEN] = {0xEC, 0x34, 0x00, 0x01, 0x5A};

    ec34u_profile_init_keyboard(&profile);
    ec34u_state_init(&state, rf_address, profile.serial);

    printk("EC34U Unifying bring-up\n");
    printk("WPID: %04x flags: %02x caps: %04x\n",
           profile.wpid, profile.caps, profile.pairing_caps);

    if (!ec34u_esb_radio_init(&radio)) {
        printk("ESB radio init failed\n");
        return 0;
    }

    printk("ESB radio init OK\n");

#if IS_ENABLED(CONFIG_EC34U_PAIR_ON_BOOT)
    printk("Attempting Unifying pairing. Receiver must be in normal pairing mode.\n");
    if (ec34u_pair_with_receiver(&radio, &profile, &state, 250)) {
        printk("Pairing frame exchange completed\n");
    } else {
        printk("Pairing frame exchange failed\n");
    }
#else
    printk("Pair-on-boot disabled. CI build is compile-only.\n");
#endif

    while (true) {
        k_sleep(K_SECONDS(5));
    }
}
