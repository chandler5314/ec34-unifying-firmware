#include "ec34u_unifying.h"

#include <stdio.h>
#include <string.h>

static void print_frame(const char *name, const uint8_t *frame, size_t len) {
    printf("%s:", name);
    for (size_t i = 0; i < len; i++) {
        printf(" %02X", frame[i]);
    }
    printf("\n");
}

int main(void) {
    struct ec34u_profile profile;
    struct ec34u_pairing_state state;
    uint8_t address[EC34U_RF_ADDR_LEN] = {0xEC, 0x34, 0x00, 0x01, 0x5A};

    ec34u_profile_init_keyboard(&profile);
    ec34u_state_init(&state, address, profile.serial);

    uint8_t req1[EC34U_PAYLOAD_LONG_LEN];
    uint8_t req2[EC34U_PAYLOAD_LONG_LEN];
    uint8_t req3[EC34U_PAYLOAD_LONG_LEN];
    uint8_t done[EC34U_PAYLOAD_MEDIUM_LEN];
    uint8_t keep_alive[EC34U_PAYLOAD_SHORT_LEN];

    ec34u_build_pairing_req1(&profile, &state, state.pair_id, req1);
    ec34u_build_pairing_req2(&profile, &state, state.pair_id, req2);
    ec34u_build_pairing_req3(&profile, state.pair_id, req3);
    ec34u_build_pairing_complete(state.pair_id, done);
    ec34u_build_keep_alive(profile.keep_alive_ms, keep_alive);

    if (!ec34u_checksum_valid(req1, sizeof(req1)) ||
        !ec34u_checksum_valid(req2, sizeof(req2)) ||
        !ec34u_checksum_valid(req3, sizeof(req3)) ||
        !ec34u_checksum_valid(done, sizeof(done)) ||
        !ec34u_checksum_valid(keep_alive, sizeof(keep_alive))) {
        fprintf(stderr, "checksum validation failed\n");
        return 1;
    }

    if (profile.wpid != EC34U_K270_WPID ||
        profile.caps != EC34U_K270_DEVICE_FLAGS ||
        profile.pairing_caps != EC34U_K270_PAIRING_CAPS) {
        fprintf(stderr, "profile constants do not match expected K270-like profile\n");
        return 1;
    }

    print_frame("pair-req1", req1, sizeof(req1));
    print_frame("pair-req2", req2, sizeof(req2));
    print_frame("pair-req3", req3, sizeof(req3));
    print_frame("pair-done", done, sizeof(done));
    print_frame("keep-alive", keep_alive, sizeof(keep_alive));

    return 0;
}
