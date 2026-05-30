#include "ec34u_radio_esb_zephyr.h"

#if defined(CONFIG_EC34U_UNIFYING_ESB)

#include <string.h>

#include <esb.h>
#include <zephyr/kernel.h>

#if CONFIG_ESB_MAX_PAYLOAD_LENGTH < EC34U_PAYLOAD_LONG_LEN
#error "CONFIG_ESB_MAX_PAYLOAD_LENGTH must be at least 22 for Unifying payloads"
#endif

K_SEM_DEFINE(ec34u_esb_tx_done, 0, 1);
K_MSGQ_DEFINE(ec34u_esb_rx_queue, sizeof(struct esb_payload), 4, 4);

static volatile bool ec34u_esb_last_tx_ok;

static void ec34u_esb_event_handler(const struct esb_evt *event) {
    if (event == NULL) {
        return;
    }

    switch (event->evt_id) {
    case ESB_EVENT_TX_SUCCESS:
        ec34u_esb_last_tx_ok = true;
        k_sem_give(&ec34u_esb_tx_done);
        break;
    case ESB_EVENT_TX_FAILED:
        ec34u_esb_last_tx_ok = false;
        esb_flush_tx();
        k_sem_give(&ec34u_esb_tx_done);
        break;
    case ESB_EVENT_RX_RECEIVED:
        while (true) {
            struct esb_payload rx;
            if (esb_read_rx_payload(&rx) != 0) {
                break;
            }
            (void)k_msgq_put(&ec34u_esb_rx_queue, &rx, K_NO_WAIT);
        }
        break;
    default:
        break;
    }
}

static bool ec34u_esb_set_channel(uint8_t channel, void *user) {
    (void)user;
    return esb_set_rf_channel(channel) == 0;
}

static bool ec34u_esb_set_address(const uint8_t address[EC34U_RF_ADDR_LEN],
                                  void *user) {
    (void)user;

    uint8_t radio_addr[EC34U_RF_ADDR_LEN];
    for (size_t i = 0; i < EC34U_RF_ADDR_LEN; i++) {
        radio_addr[i] = address[EC34U_RF_ADDR_LEN - 1 - i];
    }

    uint8_t base_addr_0[4] = {
        radio_addr[1], radio_addr[2], radio_addr[3], radio_addr[4],
    };
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t prefixes[1] = {radio_addr[0]};

    if (esb_set_address_length(EC34U_RF_ADDR_LEN) != 0) {
        return false;
    }
    if (esb_set_base_address_0(base_addr_0) != 0) {
        return false;
    }
    if (esb_set_base_address_1(base_addr_1) != 0) {
        return false;
    }
    if (esb_set_prefixes(prefixes, 1) != 0) {
        return false;
    }
    return esb_enable_pipes(0x01) == 0;
}

static bool ec34u_esb_write(const uint8_t *payload, size_t len, void *user) {
    (void)user;
    if (payload == NULL || len == 0 || len > CONFIG_ESB_MAX_PAYLOAD_LENGTH) {
        return false;
    }

    struct esb_payload tx = {
        .length = (uint8_t)len,
        .pipe = 0,
        .noack = false,
    };
    memcpy(tx.data, payload, len);

    k_sem_reset(&ec34u_esb_tx_done);
    ec34u_esb_last_tx_ok = false;

    if (esb_write_payload(&tx) != 0) {
        return false;
    }
    if (esb_start_tx() != 0) {
        return false;
    }
    if (k_sem_take(&ec34u_esb_tx_done, K_MSEC(30)) != 0) {
        return false;
    }
    return ec34u_esb_last_tx_ok;
}

static int ec34u_esb_read(uint8_t *payload, size_t max_len,
                          uint32_t timeout_ms, void *user) {
    (void)user;
    if (payload == NULL || max_len == 0) {
        return -EINVAL;
    }

    struct esb_payload rx;
    if (k_msgq_get(&ec34u_esb_rx_queue, &rx, K_MSEC(timeout_ms)) != 0) {
        return 0;
    }
    if (rx.length > max_len) {
        return -EMSGSIZE;
    }
    memcpy(payload, rx.data, rx.length);
    return rx.length;
}

bool ec34u_esb_radio_init(struct ec34u_radio_ops *ops) {
    if (ops == NULL) {
        return false;
    }

    struct esb_config config = ESB_DEFAULT_CONFIG;
    config.protocol = ESB_PROTOCOL_ESB_DPL;
    config.mode = ESB_MODE_PTX;
    config.event_handler = ec34u_esb_event_handler;
    config.bitrate = ESB_BITRATE_2MBPS;
    config.crc = ESB_CRC_16BIT;
    config.tx_output_power = ESB_TX_POWER_0DBM;
    config.retransmit_delay = 600;
    config.retransmit_count = 15;
    config.tx_mode = ESB_TXMODE_MANUAL;
    config.payload_length = EC34U_PAYLOAD_LONG_LEN;
    config.selective_auto_ack = false;

    if (esb_init(&config) != 0) {
        return false;
    }
    if (!ec34u_esb_set_address((const uint8_t[EC34U_RF_ADDR_LEN]){
            0xBB, 0x0A, 0xDC, 0xA5, 0x75,
        }, NULL)) {
        return false;
    }
    if (!ec34u_esb_set_channel(ec34u_pairing_channels[0], NULL)) {
        return false;
    }

    ops->set_channel = ec34u_esb_set_channel;
    ops->set_address = ec34u_esb_set_address;
    ops->write = ec34u_esb_write;
    ops->read = ec34u_esb_read;
    ops->user = NULL;
    return true;
}

#else

bool ec34u_esb_radio_init(struct ec34u_radio_ops *ops) {
    (void)ops;
    return false;
}

#endif
