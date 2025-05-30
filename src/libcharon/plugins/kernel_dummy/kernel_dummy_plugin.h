/*
 * Dummy kernel plugin for DPDK integration
 */

#ifndef KERNEL_DUMMY_PLUGIN_H_
#define KERNEL_DUMMY_PLUGIN_H_

#include <plugins/plugin.h>

typedef struct kernel_dummy_plugin_t kernel_dummy_plugin_t;

/**
 * Dummy kernel interface plugin
 */
struct kernel_dummy_plugin_t {
	/**
	 * implements plugin interface
	 */
	plugin_t plugin;
};

/**
 * Create a dummy kernel plugin instance.
 */
plugin_t *kernel_dummy_plugin_create();

#endif /** KERNEL_DUMMY_PLUGIN_H_ */ 