/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic mesh light sample
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/dk_prov.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "smp_dfu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	dk_leds_init();
	dk_buttons_init(NULL);

	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init());
	if (err) {
		LOG_ERR("Initializing mesh failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	LOG_INF("Mesh initialized\n");

	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF52X) &&
	    IS_ENABLED(CONFIG_MCUMGR_SMP_BT)) {
		err = smp_dfu_init();
		if (err) {
			LOG_ERR("Unable to initialize DFU (err %d)\n", err);
		}
	}
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    (pressed & changed & BIT(3))) {
		bt_mesh_proxy_identity_enable();
		return;
	}

	if (pressed & changed & BIT(0)) {
		// start IV
		static bool isEnabled = false;
		bt_mesh_iv_update_test(isEnabled = !isEnabled);
		LOG_INF("bt_mesh_iv_update_test >> set to %s",
			isEnabled ? "ENABLED" : "DISABLED");
	} else if (pressed & changed & BIT(1)) {
		// force update
		LOG_INF("bt_mesh_iv_update >> status %s",
			bt_mesh_iv_update() ? "OK" : "FAILED");
	}
}

void main(void)
{
	int err;

	LOG_INF("Initializing...\n");

	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
	}

	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);
}
