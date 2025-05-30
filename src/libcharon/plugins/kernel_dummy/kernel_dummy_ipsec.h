/*
 * Dummy kernel ipsec interface for DPDK integration
 */

#ifndef KERNEL_DUMMY_IPSEC_H_
#define KERNEL_DUMMY_IPSEC_H_

#include <kernel/kernel_ipsec.h>

typedef struct kernel_dummy_ipsec_t kernel_dummy_ipsec_t;

/**
 * Dummy implementation of the ipsec interface
 */
struct kernel_dummy_ipsec_t {
	/**
	 * Implements kernel_ipsec_t interface
	 */
	kernel_ipsec_t interface;
};

/**
 * Create a dummy ipsec interface instance.
 *
 * @return	kernel_dummy_ipsec_t instance
 */
kernel_dummy_ipsec_t *kernel_dummy_ipsec_create();

#endif /** KERNEL_DUMMY_IPSEC_H_ */ 