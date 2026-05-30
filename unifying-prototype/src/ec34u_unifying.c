#include "ec34u_unifying.h"

#include <string.h>

const uint8_t ec34u_pairing_channels[11] = {
    62, 8, 35, 65, 14, 41, 71, 17, 44, 74, 5,
};

const uint8_t ec34u_data_channels[25] = {
    5, 8, 11, 14, 17, 20, 23, 26, 29, 32,
    35, 38, 41, 44, 47, 50, 53, 56, 59, 62,
    65, 68, 71, 74, 77,
};

static void put_u16_be(uint8_t *dst, uint16_t value) {
    dst[0] = (uint8_t)(value >> 8);
    dst[1] = (uint8_t)value;
}

static void put_u32_be(uint8_t *dst, uint32_t value) {
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static uint16_t get_u16_be(const uint8_t *src) {
    return ((uint16_t)src[0] << 8) | src[1];
}

static uint32_t get_u32_be(const uint8_t *src) {
    return ((uint32_t)src[0] << 24) |
           ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] << 8) |
           src[3];
}

static size_t bounded_strlen(const char *text, size_t max_len) {
    size_t len = 0;
    while (len < max_len && text[len] != '\0') {
        len++;
    }
    return len;
}

static const uint8_t ec34u_pairing_address[EC34U_RF_ADDR_LEN] = {
    0xBB, 0x0A, 0xDC, 0xA5, 0x75,
};

static bool radio_ops_valid(const struct ec34u_radio_ops *radio) {
    return radio != NULL &&
           radio->set_channel != NULL &&
           radio->set_address != NULL &&
           radio->write != NULL &&
           radio->read != NULL;
}

void ec34u_profile_init_keyboard(struct ec34u_profile *profile) {
    memset(profile, 0, sizeof(*profile));
    profile->wpid = EC34U_K270_WPID; /* K270-like HID++ 2.0 keyboard profile for first bring-up. */
    profile->protocol = 0x04;
    profile->device_type = EC34U_DEVICE_TYPE_KEYBOARD;
    profile->caps = EC34U_K270_DEVICE_FLAGS;
    profile->pairing_caps = EC34U_K270_PAIRING_CAPS;
    profile->keep_alive_ms = 0x0014;
    profile->report_types = EC34U_REPORT_KEYBOARD |
                            EC34U_REPORT_MULTIMEDIA |
                            EC34U_REPORT_POWER_KEYS |
                            EC34U_REPORT_KEYBOARD_LED |
                            EC34U_REPORT_HIDPP_SHORT |
                            EC34U_REPORT_HIDPP_LONG |
                            EC34U_REPORT_ENCRYPTED_KEYBOARD;
    profile->firmware_version = 0x35000017;
    profile->serial = 0xEC340001;
    profile->power_switch_location = 0x03;
    memcpy(profile->name, "EC34U", 6);
}

void ec34u_state_init(struct ec34u_pairing_state *state,
                      const uint8_t rf_address[EC34U_RF_ADDR_LEN],
                      uint32_t serial) {
    memset(state, 0, sizeof(*state));
    memcpy(state->rf_address, rf_address, EC34U_RF_ADDR_LEN);
    state->serial = serial;
    state->device_nonce = 0xEC340001;
    state->pair_id = rf_address[0];
}

uint8_t ec34u_checksum(const uint8_t *payload, size_t len_without_checksum) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len_without_checksum; i++) {
        sum = (uint8_t)(sum + payload[i]);
    }
    return (uint8_t)(0u - sum);
}

bool ec34u_checksum_valid(const uint8_t *payload, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + payload[i]);
    }
    return sum == 0;
}

void ec34u_finalize(uint8_t *payload, size_t len) {
    if (len == 0) {
        return;
    }
    payload[len - 1] = ec34u_checksum(payload, len - 1);
}

static bool pairing_rsp_common(const uint8_t *payload, size_t len,
                               uint8_t expected_step) {
    if (payload == NULL || len < EC34U_PAYLOAD_MEDIUM_LEN) {
        return false;
    }
    if (!ec34u_checksum_valid(payload, len)) {
        return false;
    }
    if ((payload[1] & EC34U_REPORT_PAIRING) == 0) {
        return false;
    }
    return payload[2] == expected_step;
}

void ec34u_build_pairing_req1(const struct ec34u_profile *profile,
                              const struct ec34u_pairing_state *state,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_LONG_LEN);
    out[0] = prefix;
    out[1] = EC34U_REPORT_PAIRING | EC34U_REPORT_KEEP_ALIVE;
    out[2] = 0x01;
    out[3] = state->rf_address[4];
    out[4] = state->rf_address[3];
    out[5] = state->rf_address[2];
    out[6] = state->rf_address[1];
    out[7] = state->rf_address[0];
    out[8] = (uint8_t)profile->keep_alive_ms;
    put_u16_be(&out[9], profile->wpid);
    out[11] = profile->protocol;
    out[12] = 0x00;
    out[13] = profile->device_type;
    out[14] = profile->caps;
    out[20] = 0x1A;
    ec34u_finalize(out, EC34U_PAYLOAD_LONG_LEN);
}

void ec34u_build_pairing_complete(uint8_t prefix,
                                  uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_MEDIUM_LEN);
    out[0] = prefix;
    out[1] = EC34U_REPORT_PAIRING | EC34U_REPORT_KEEP_ALIVE;
    out[2] = 0x07;
    out[3] = 0x01;
    ec34u_finalize(out, EC34U_PAYLOAD_MEDIUM_LEN);
}

bool ec34u_parse_pairing_rsp1(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp1 *out) {
    if (!pairing_rsp_common(payload, len, 0x01) ||
        len != EC34U_PAYLOAD_LONG_LEN ||
        state == NULL || out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->pair_id = payload[0];
    memcpy(out->address, &payload[3], EC34U_RF_ADDR_LEN);
    out->timeout = payload[8];
    out->dongle_wpid = get_u16_be(&payload[9]);
    out->dongle_device_type = get_u16_be(&payload[13]);

    state->pair_id = out->pair_id;
    memcpy(state->rf_address, out->address, EC34U_RF_ADDR_LEN);
    state->dongle_wpid = out->dongle_wpid;
    state->dongle_device_type = out->dongle_device_type;
    state->pair_step = 1;
    return true;
}

bool ec34u_parse_pairing_rsp2(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp2 *out) {
    if (!pairing_rsp_common(payload, len, 0x02) ||
        len != EC34U_PAYLOAD_LONG_LEN ||
        state == NULL || out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->dongle_nonce = get_u32_be(&payload[3]);
    out->dongle_serial = get_u32_be(&payload[7]);
    out->capabilities = get_u16_be(&payload[11]);

    state->dongle_nonce = out->dongle_nonce;
    state->pair_step = 2;
    return true;
}

bool ec34u_parse_pairing_rsp3(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp3 *out) {
    if (!pairing_rsp_common(payload, len, 0x06) ||
        len != EC34U_PAYLOAD_MEDIUM_LEN ||
        state == NULL || out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    memcpy(out->key_material, &payload[3], sizeof(out->key_material));
    state->pair_step = 3;
    return true;
}

void ec34u_build_pairing_req2(const struct ec34u_profile *profile,
                              const struct ec34u_pairing_state *state,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_LONG_LEN);
    out[0] = prefix;
    out[1] = EC34U_REPORT_PAIRING | EC34U_REPORT_KEEP_ALIVE;
    out[2] = 0x02;
    put_u32_be(&out[3], state->device_nonce);
    put_u32_be(&out[7], state->serial);
    put_u16_be(&out[11], profile->pairing_caps);
    out[15] = profile->power_switch_location;
    ec34u_finalize(out, EC34U_PAYLOAD_LONG_LEN);
}

void ec34u_build_pairing_req3(const struct ec34u_profile *profile,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]) {
    size_t name_len = bounded_strlen(profile->name, sizeof(profile->name));
    if (name_len > 16) {
        name_len = 16;
    }

    memset(out, 0, EC34U_PAYLOAD_LONG_LEN);
    out[0] = prefix;
    out[1] = EC34U_REPORT_PAIRING | EC34U_REPORT_KEEP_ALIVE;
    out[2] = 0x03;
    out[3] = 0x01;
    out[4] = (uint8_t)name_len;
    memcpy(&out[5], profile->name, name_len);
    ec34u_finalize(out, EC34U_PAYLOAD_LONG_LEN);
}

void ec34u_build_keep_alive(uint16_t keep_alive_ms,
                            uint8_t out[EC34U_PAYLOAD_SHORT_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_SHORT_LEN);
    out[1] = EC34U_REPORT_KEEP_ALIVE;
    put_u16_be(&out[2], keep_alive_ms);
    ec34u_finalize(out, EC34U_PAYLOAD_SHORT_LEN);
}

void ec34u_build_short_wake(uint8_t rf_index,
                            uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_MEDIUM_LEN);
    out[0] = rf_index;
    out[1] = EC34U_REPORT_KEYBOARD | EC34U_REPORT_KEEP_ALIVE;
    out[2] = 0x01;
    out[3] = 0x4B;
    out[4] = 0x01;
    ec34u_finalize(out, EC34U_PAYLOAD_MEDIUM_LEN);
}

void ec34u_build_long_wake(uint8_t rf_index,
                           uint8_t out[EC34U_PAYLOAD_LONG_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_LONG_LEN);
    out[0] = rf_index;
    out[1] = EC34U_REPORT_HIDPP_LONG | EC34U_REPORT_KEEP_ALIVE;
    out[2] = rf_index;
    out[3] = 0x01;
    out[5] = 0x01;
    out[6] = 0x01;
    out[7] = 0x01;
    ec34u_finalize(out, EC34U_PAYLOAD_LONG_LEN);
}

void ec34u_build_keyboard_plain(const struct ec34u_keyboard_report *report,
                                uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]) {
    memset(out, 0, EC34U_PAYLOAD_MEDIUM_LEN);
    out[1] = EC34U_REPORT_KEYBOARD | EC34U_REPORT_KEEP_ALIVE | 0x80;
    out[2] = report->modifiers;
    out[3] = report->keys[0];
    out[4] = report->keys[1];
    out[5] = report->keys[2];
    out[6] = report->keys[3];
    out[7] = report->keys[4];
    out[8] = report->keys[5];
    ec34u_finalize(out, EC34U_PAYLOAD_MEDIUM_LEN);
}

bool ec34u_send_keep_alive(const struct ec34u_radio_ops *radio,
                           const struct ec34u_profile *profile) {
    uint8_t frame[EC34U_PAYLOAD_SHORT_LEN];
    ec34u_build_keep_alive(profile->keep_alive_ms, frame);
    return radio->write(frame, sizeof(frame), radio->user);
}

bool ec34u_send_keyboard_plain(const struct ec34u_radio_ops *radio,
                               const struct ec34u_keyboard_report *report) {
    uint8_t frame[EC34U_PAYLOAD_MEDIUM_LEN];
    ec34u_build_keyboard_plain(report, frame);
    return radio->write(frame, sizeof(frame), radio->user);
}

static bool ec34u_read_pairing_rsp1(const struct ec34u_radio_ops *radio,
                                    struct ec34u_pairing_state *state,
                                    uint32_t timeout_ms) {
    uint8_t rx[EC34U_PAYLOAD_LONG_LEN];
    int len = radio->read(rx, sizeof(rx), timeout_ms, radio->user);
    struct ec34u_pairing_rsp1 rsp;
    return len == EC34U_PAYLOAD_LONG_LEN &&
           ec34u_parse_pairing_rsp1(state, rx, (size_t)len, &rsp);
}

static bool ec34u_read_pairing_rsp2(const struct ec34u_radio_ops *radio,
                                    struct ec34u_pairing_state *state,
                                    uint32_t timeout_ms) {
    uint8_t rx[EC34U_PAYLOAD_LONG_LEN];
    int len = radio->read(rx, sizeof(rx), timeout_ms, radio->user);
    struct ec34u_pairing_rsp2 rsp;
    return len == EC34U_PAYLOAD_LONG_LEN &&
           ec34u_parse_pairing_rsp2(state, rx, (size_t)len, &rsp);
}

static bool ec34u_read_pairing_rsp3(const struct ec34u_radio_ops *radio,
                                    struct ec34u_pairing_state *state,
                                    uint32_t timeout_ms) {
    uint8_t rx[EC34U_PAYLOAD_MEDIUM_LEN];
    int len = radio->read(rx, sizeof(rx), timeout_ms, radio->user);
    struct ec34u_pairing_rsp3 rsp;
    return len == EC34U_PAYLOAD_MEDIUM_LEN &&
           ec34u_parse_pairing_rsp3(state, rx, (size_t)len, &rsp);
}

bool ec34u_pair_with_receiver(const struct ec34u_radio_ops *radio,
                              const struct ec34u_profile *profile,
                              struct ec34u_pairing_state *state,
                              uint32_t response_timeout_ms) {
    if (!radio_ops_valid(radio) || profile == NULL || state == NULL) {
        return false;
    }

    if (!radio->set_address(ec34u_pairing_address, radio->user)) {
        return false;
    }

    uint8_t tx_long[EC34U_PAYLOAD_LONG_LEN];
    uint8_t tx_medium[EC34U_PAYLOAD_MEDIUM_LEN];
    uint8_t tx_short[EC34U_PAYLOAD_SHORT_LEN];

    bool got_rsp1 = false;
    for (size_t i = 0; i < sizeof(ec34u_pairing_channels); i++) {
        if (!radio->set_channel(ec34u_pairing_channels[i], radio->user)) {
            continue;
        }
        ec34u_build_pairing_req1(profile, state, state->pair_id, tx_long);
        if (!radio->write(tx_long, sizeof(tx_long), radio->user)) {
            continue;
        }
        ec34u_build_keep_alive(profile->keep_alive_ms, tx_short);
        (void)radio->write(tx_short, sizeof(tx_short), radio->user);

        if (ec34u_read_pairing_rsp1(radio, state, response_timeout_ms)) {
            got_rsp1 = true;
            break;
        }
    }
    if (!got_rsp1) {
        return false;
    }

    if (!radio->set_address(state->rf_address, radio->user)) {
        return false;
    }

    ec34u_build_pairing_req2(profile, state, state->pair_id, tx_long);
    if (!radio->write(tx_long, sizeof(tx_long), radio->user)) {
        return false;
    }
    ec34u_build_keep_alive(profile->keep_alive_ms, tx_short);
    (void)radio->write(tx_short, sizeof(tx_short), radio->user);
    if (!ec34u_read_pairing_rsp2(radio, state, response_timeout_ms)) {
        return false;
    }

    ec34u_build_pairing_req3(profile, state->pair_id, tx_long);
    if (!radio->write(tx_long, sizeof(tx_long), radio->user)) {
        return false;
    }
    ec34u_build_keep_alive(profile->keep_alive_ms, tx_short);
    (void)radio->write(tx_short, sizeof(tx_short), radio->user);
    if (!ec34u_read_pairing_rsp3(radio, state, response_timeout_ms)) {
        return false;
    }

    ec34u_build_pairing_complete(state->pair_id, tx_medium);
    if (!radio->write(tx_medium, sizeof(tx_medium), radio->user)) {
        return false;
    }

    state->paired = true;
    return true;
}
