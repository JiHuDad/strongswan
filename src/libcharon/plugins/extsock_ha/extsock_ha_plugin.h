/*
 * Copyright (C) 2024 strongSwan Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup extsock_ha_plugin extsock_ha_plugin
 * @{ @ingroup cplugins
 */

#ifndef EXTSOCK_HA_PLUGIN_H_
#define EXTSOCK_HA_PLUGIN_H_

#include <plugins/plugin.h>

typedef struct extsock_ha_plugin_t extsock_ha_plugin_t;

/**
 * External socket plugin with High Availability (HA) support for SEGW failover.
 *
 * This plugin provides all the functionality of the original extsock plugin plus:
 * - External socket communication for IPsec configuration
 * - JSON-based configuration management
 * - Event notifications to external applications
 * - Automatic SEGW (Security Gateway) failover
 * - Complete peer configuration management with HA
 * - Event-driven tunnel state monitoring
 * - Thread-safe HA configuration management
 */
struct extsock_ha_plugin_t {

    /**
     * Implements plugin interface.
     */
    plugin_t plugin;
};

/**
 * Create an extsock_ha_plugin instance.
 *
 * @return          plugin instance
 */
plugin_t *extsock_ha_plugin_create();

#endif /** EXTSOCK_HA_PLUGIN_H_ @}*/ 