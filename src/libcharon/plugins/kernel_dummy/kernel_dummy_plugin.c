#include "kernel_dummy_plugin.h"
#include "kernel_dummy_ipsec.h"
#include "kernel_dummy_net.h"

#include <stdlib.h>

typedef struct private_kernel_dummy_plugin_t private_kernel_dummy_plugin_t;

struct private_kernel_dummy_plugin_t {
	kernel_dummy_plugin_t public;
};

METHOD(plugin_t, get_name, char*,
	private_kernel_dummy_plugin_t *this)
{
	return "kernel-dummy";
}

METHOD(plugin_t, get_features, int,
	private_kernel_dummy_plugin_t *this, plugin_feature_t *features[])
{
	static plugin_feature_t f[] = {
		PLUGIN_CALLBACK(kernel_ipsec_register, kernel_dummy_ipsec_create),
			PLUGIN_PROVIDE(CUSTOM, "kernel-ipsec"),
		PLUGIN_CALLBACK(kernel_net_register, kernel_dummy_net_create),
			PLUGIN_PROVIDE(CUSTOM, "kernel-net"),
	};
	*features = f;
	return countof(f);
}

METHOD(plugin_t, destroy, void,
	private_kernel_dummy_plugin_t *this)
{
	free(this);
}

plugin_t *kernel_dummy_plugin_create()
{
	private_kernel_dummy_plugin_t *this;

	INIT(this,
		.public = {
			.plugin = {
				.get_name = _get_name,
				.get_features = _get_features,
				.destroy = _destroy,
			},
		},
	);

	return &this->public.plugin;
} 