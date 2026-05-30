#include "ec34u_unifying.h"

#include <string.h>

/*
 * Placeholder for encrypted keyboard reports.
 *
 * This project only targets legitimate pairing with the user's own Unifying
 * receiver. The link key must come from that pairing flow. Do not add support
 * for sniffed keys, forced pairing, or injection against unknown receivers here.
 */

bool ec34u_crypto_available(void) {
    return false;
}

bool ec34u_build_keyboard_encrypted(const struct ec34u_pairing_state *state,
                                    const struct ec34u_keyboard_report *report,
                                    uint32_t counter,
                                    uint8_t out[EC34U_PAYLOAD_LONG_LEN]) {
    (void)state;
    (void)report;
    (void)counter;
    memset(out, 0, EC34U_PAYLOAD_LONG_LEN);
    return false;
}
