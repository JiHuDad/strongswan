/*
 * Copyright (C) 2024 strongSwan Project
 */

/**
 * @defgroup extsock extsock
 * @ingroup cplugins
 * @{
 */

#ifndef EXTSOCK_PLUGIN_H_
#define EXTSOCK_PLUGIN_H_

#include <plugins/plugin.h>

typedef struct extsock_plugin_t extsock_plugin_t;

/**
 * External Socket plugin interface
 */
struct extsock_plugin_t {
    
    /**
     * Implements plugin interface
     */
    plugin_t plugin;
};

/**
 * Create extsock_plugin instance.
 */
plugin_t *extsock_plugin_create();

#endif /** EXTSOCK_PLUGIN_H_ @}*/ 