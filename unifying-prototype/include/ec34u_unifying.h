#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EC34U_PAYLOAD_SHORT_LEN 5u
#define EC34U_PAYLOAD_MEDIUM_LEN 10u
#define EC34U_PAYLOAD_LONG_LEN 22u
#define EC34U_RF_ADDR_LEN 5u
#define EC34U_LINK_KEY_LEN 16u

#define EC34U_PAIRING_ADDR UINT64_C(0xBB0ADCA575)
#define EC34U_K270_WPID 0x4003u
#define EC34U_K270_DEVICE_FLAGS 0x0Du
#define EC34U_K270_PAIRING_CAPS 0x1A40u

enum ec34u_report_type {
    EC34U_REPORT_KEYBOARD = 0x01,
    EC34U_REPORT_MOUSE = 0x02,
    EC34U_REPORT_MULTIMEDIA = 0x03,
    EC34U_REPORT_POWER_KEYS = 0x04,
    EC34U_REPORT_KEYBOARD_LED = 0x0E,
    EC34U_REPORT_SET_KEEP_ALIVE = 0x0F,
    EC34U_REPORT_HIDPP_SHORT = 0x10,
    EC34U_REPORT_HIDPP_LONG = 0x11,
    EC34U_REPORT_ENCRYPTED_KEYBOARD = 0x13,
    EC34U_REPORT_ENCRYPTED_HIDPP_LONG = 0x1B,
    EC34U_REPORT_PAIRING = 0x1F,
    EC34U_REPORT_KEEP_ALIVE = 0x40,
};

enum ec34u_device_type {
    EC34U_DEVICE_TYPE_KEYBOARD = 0x01,
    EC34U_DEVICE_TYPE_MOUSE = 0x02,
    EC34U_DEVICE_TYPE_NUMPAD = 0x03,
};

enum ec34u_device_caps {
    EC34U_CAP_LINK_ENCRYPTION = 0x01,
    EC34U_CAP_BATTERY_STATUS = 0x02,
    EC34U_CAP_UNIFYING_COMPATIBLE = 0x04,
    EC34U_CAP_RESERVED_3 = 0x08,
};

struct ec34u_profile {
    uint16_t wpid;
    uint8_t protocol;
    uint8_t device_type;
    uint8_t caps;
    uint16_t pairing_caps;
    uint16_t keep_alive_ms;
    uint32_t report_types;
    uint32_t firmware_version;
    uint32_t serial;
    uint8_t power_switch_location;
    char name[16];
};

struct ec34u_keyboard_report {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
};

struct ec34u_pairing_state {
    uint8_t rf_address[EC34U_RF_ADDR_LEN];
    uint8_t link_key[EC34U_LINK_KEY_LEN];
    uint32_t device_nonce;
    uint32_t dongle_nonce;
    uint32_t serial;
    uint16_t dongle_wpid;
    uint16_t dongle_device_type;
    uint8_t pair_id;
    uint8_t pair_step;
    bool paired;
};

struct ec34u_pairing_rsp1 {
    uint8_t pair_id;
    uint8_t address[EC34U_RF_ADDR_LEN];
    uint8_t timeout;
    uint16_t dongle_wpid;
    uint16_t dongle_device_type;
};

struct ec34u_pairing_rsp2 {
    uint32_t dongle_nonce;
    uint32_t dongle_serial;
    uint16_t capabilities;
};

struct ec34u_pairing_rsp3 {
    uint8_t key_material[6];
};

struct ec34u_radio_ops {
    bool (*set_channel)(uint8_t channel, void *user);
    bool (*set_address)(const uint8_t address[EC34U_RF_ADDR_LEN], void *user);
    bool (*write)(const uint8_t *payload, size_t len, void *user);
    int (*read)(uint8_t *payload, size_t max_len, uint32_t timeout_ms, void *user);
    void *user;
};

extern const uint8_t ec34u_pairing_channels[11];
extern const uint8_t ec34u_data_channels[25];

void ec34u_profile_init_keyboard(struct ec34u_profile *profile);
void ec34u_state_init(struct ec34u_pairing_state *state,
                      const uint8_t rf_address[EC34U_RF_ADDR_LEN],
                      uint32_t serial);

uint8_t ec34u_checksum(const uint8_t *payload, size_t len_without_checksum);
bool ec34u_checksum_valid(const uint8_t *payload, size_t len);
void ec34u_finalize(uint8_t *payload, size_t len);

void ec34u_build_pairing_req1(const struct ec34u_profile *profile,
                              const struct ec34u_pairing_state *state,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]);
void ec34u_build_pairing_req2(const struct ec34u_profile *profile,
                              const struct ec34u_pairing_state *state,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]);
void ec34u_build_pairing_req3(const struct ec34u_profile *profile,
                              uint8_t prefix,
                              uint8_t out[EC34U_PAYLOAD_LONG_LEN]);
void ec34u_build_pairing_complete(uint8_t prefix,
                                  uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]);
bool ec34u_parse_pairing_rsp1(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp1 *out);
bool ec34u_parse_pairing_rsp2(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp2 *out);
bool ec34u_parse_pairing_rsp3(struct ec34u_pairing_state *state,
                              const uint8_t *payload,
                              size_t len,
                              struct ec34u_pairing_rsp3 *out);
void ec34u_build_keep_alive(uint16_t keep_alive_ms,
                            uint8_t out[EC34U_PAYLOAD_SHORT_LEN]);
void ec34u_build_short_wake(uint8_t rf_index,
                            uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]);
void ec34u_build_long_wake(uint8_t rf_index,
                           uint8_t out[EC34U_PAYLOAD_LONG_LEN]);
void ec34u_build_keyboard_plain(const struct ec34u_keyboard_report *report,
                                uint8_t out[EC34U_PAYLOAD_MEDIUM_LEN]);

bool ec34u_send_keep_alive(const struct ec34u_radio_ops *radio,
                           const struct ec34u_profile *profile);
bool ec34u_send_keyboard_plain(const struct ec34u_radio_ops *radio,
                               const struct ec34u_keyboard_report *report);
bool ec34u_pair_with_receiver(const struct ec34u_radio_ops *radio,
                              const struct ec34u_profile *profile,
                              struct ec34u_pairing_state *state,
                              uint32_t response_timeout_ms);

bool ec34u_crypto_available(void);
bool ec34u_build_keyboard_encrypted(const struct ec34u_pairing_state *state,
                                    const struct ec34u_keyboard_report *report,
                                    uint32_t counter,
                                    uint8_t out[EC34U_PAYLOAD_LONG_LEN]);

#ifdef __cplusplus
}
#endif
