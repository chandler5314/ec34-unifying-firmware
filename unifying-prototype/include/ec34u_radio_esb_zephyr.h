#pragma once

#include "ec34u_unifying.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Optional Zephyr/NCS ESB adapter.
 *
 * This header is intentionally separate from ec34u_unifying.h so the protocol
 * core remains portable and can still be built without Zephyr or NCS.
 */
bool ec34u_esb_radio_init(struct ec34u_radio_ops *ops);

#ifdef __cplusplus
}
#endif
