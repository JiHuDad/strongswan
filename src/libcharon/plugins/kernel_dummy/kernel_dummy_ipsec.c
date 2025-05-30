#include "kernel_dummy_ipsec.h"
#include <stdlib.h>
#include <time.h>

static kernel_feature_t get_features(kernel_ipsec_t *this) { return 0; }
static status_t get_spi(kernel_ipsec_t *this, host_t *src, host_t *dst, uint8_t protocol, uint32_t *spi) { *spi = 0xdeadbeef; return SUCCESS; }
static status_t get_cpi(kernel_ipsec_t *this, host_t *src, host_t *dst, uint16_t *cpi) { *cpi = 0; return SUCCESS; }
static status_t add_sa(kernel_ipsec_t *this, kernel_ipsec_sa_id_t *id, kernel_ipsec_add_sa_t *data) { return SUCCESS; }
static status_t update_sa(kernel_ipsec_t *this, kernel_ipsec_sa_id_t *id, kernel_ipsec_update_sa_t *data) { return SUCCESS; }
static status_t query_sa(kernel_ipsec_t *this, kernel_ipsec_sa_id_t *id, kernel_ipsec_query_sa_t *data, uint64_t *bytes, uint64_t *packets, time_t *time) { if (bytes) *bytes = 0; if (packets) *packets = 0; if (time) *time = 0; return SUCCESS; }
static status_t del_sa(kernel_ipsec_t *this, kernel_ipsec_sa_id_t *id, kernel_ipsec_del_sa_t *data) { return SUCCESS; }
static status_t flush_sas(kernel_ipsec_t *this) { return SUCCESS; }
static status_t add_policy(kernel_ipsec_t *this, kernel_ipsec_policy_id_t *id, kernel_ipsec_manage_policy_t *data) { return SUCCESS; }
static status_t query_policy(kernel_ipsec_t *this, kernel_ipsec_policy_id_t *id, kernel_ipsec_query_policy_t *data, time_t *use_time) { if (use_time) *use_time = 0; return SUCCESS; }
static status_t del_policy(kernel_ipsec_t *this, kernel_ipsec_policy_id_t *id, kernel_ipsec_manage_policy_t *data) { return SUCCESS; }
static status_t flush_policies(kernel_ipsec_t *this) { return SUCCESS; }
static bool bypass_socket(kernel_ipsec_t *this, int fd, int family) { return TRUE; }
static bool enable_udp_decap(kernel_ipsec_t *this, int fd, int family, uint16_t port) { return TRUE; }
static void destroy(kernel_ipsec_t *this) { free(this); }

kernel_dummy_ipsec_t *kernel_dummy_ipsec_create()
{
	kernel_dummy_ipsec_t *this = calloc(1, sizeof(kernel_dummy_ipsec_t));
	this->interface.get_features = get_features;
	this->interface.get_spi = get_spi;
	this->interface.get_cpi = get_cpi;
	this->interface.add_sa = add_sa;
	this->interface.update_sa = update_sa;
	this->interface.query_sa = query_sa;
	this->interface.del_sa = del_sa;
	this->interface.flush_sas = flush_sas;
	this->interface.add_policy = add_policy;
	this->interface.query_policy = query_policy;
	this->interface.del_policy = del_policy;
	this->interface.flush_policies = flush_policies;
	this->interface.bypass_socket = bypass_socket;
	this->interface.enable_udp_decap = enable_udp_decap;
	this->interface.destroy = destroy;
	return this;
} 