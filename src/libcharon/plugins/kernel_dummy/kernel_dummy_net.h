/*
 * Dummy kernel net interface for DPDK integration
 */

#ifndef KERNEL_DUMMY_NET_H_
#define KERNEL_DUMMY_NET_H_

#include <kernel/kernel_net.h>

typedef struct kernel_dummy_net_t kernel_dummy_net_t;

/**
 * Dummy implementation of the net interface
 */
struct kernel_dummy_net_t {
	/**
	 * Implements kernel_net_t interface
	 */
	kernel_net_t interface;
};

/**
 * Create a dummy net interface instance.
 *
 * @return	kernel_dummy_net_t instance
 */
kernel_dummy_net_t *kernel_dummy_net_create();

#endif /** KERNEL_DUMMY_NET_H_ */ 