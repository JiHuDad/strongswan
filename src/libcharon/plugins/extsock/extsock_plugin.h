#ifndef EXTSOCK_PLUGIN_H_
#define EXTSOCK_PLUGIN_H_

#include <plugins/plugin.h>

typedef struct extsock_plugin_t extsock_plugin_t;

/**
 * Public interface of extsock plugin.
 */
struct extsock_plugin_t {
    /**
     * Implements plugin interface
     */
    plugin_t plugin;
};

/**
 * Create the extsock plugin instance
 */
plugin_t *extsock_plugin_create();

#endif /* EXTSOCK_PLUGIN_H_ */ 