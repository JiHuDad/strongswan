#include "kernel_dummy_net.h"
#include <stdlib.h>

static kernel_feature_t get_features(kernel_net_t *this) { return 0; }
static host_t* get_source_addr(kernel_net_t *this, host_t *dest, host_t *src) { return NULL; }
static host_t* get_nexthop(kernel_net_t *this, host_t *dest, int prefix, host_t *src, char **iface) { if (iface) *iface = NULL; return NULL; }
static bool get_interface(kernel_net_t *this, host_t *host, char **name) { if (name) *name = NULL; return FALSE; }
static enumerator_t* create_address_enumerator(kernel_net_t *this, kernel_address_type_t which) { return NULL; }
static enumerator_t* create_local_subnet_enumerator(kernel_net_t *this) { return NULL; }
static status_t add_ip(kernel_net_t *this, host_t *virtual_ip, int prefix, char *iface) { return SUCCESS; }
static status_t del_ip(kernel_net_t *this, host_t *virtual_ip, int prefix, bool wait) { return SUCCESS; }
static status_t add_route(kernel_net_t *this, chunk_t dst_net, uint8_t prefixlen, host_t *gateway, host_t *src_ip, char *if_name, bool pass) { return SUCCESS; }
static status_t del_route(kernel_net_t *this, chunk_t dst_net, uint8_t prefixlen, host_t *gateway, host_t *src_ip, char *if_name, bool pass) { return SUCCESS; }
static void destroy(kernel_net_t *this) { free(this); }

kernel_dummy_net_t *kernel_dummy_net_create()
{
	kernel_dummy_net_t *this = calloc(1, sizeof(kernel_dummy_net_t));
	this->interface.get_features = get_features;
	this->interface.get_source_addr = get_source_addr;
	this->interface.get_nexthop = get_nexthop;
	this->interface.get_interface = get_interface;
	this->interface.create_address_enumerator = create_address_enumerator;
	this->interface.create_local_subnet_enumerator = create_local_subnet_enumerator;
	this->interface.add_ip = add_ip;
	this->interface.del_ip = del_ip;
	this->interface.add_route = add_route;
	this->interface.del_route = del_route;
	this->interface.destroy = destroy;
	return this;
} 