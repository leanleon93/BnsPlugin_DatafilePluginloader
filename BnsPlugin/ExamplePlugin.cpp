#include "DatafilePluginsdk.h"

static PluginReturnData __fastcall DatafileItemDetour(PluginParams* params) {
	if (params == nullptr || params->table == nullptr || params->dataManager == nullptr) {
		return {};
	}
	// Example: Return a specific item for a known key
	// Hongmoon's Blessing Recovery Potion -> ncoin test item
	unsigned __int64 key = params->key;
	if (key == 4295902840) {
		DrEl* result = params->oFind_b8(params->table, 4294967396);
		return { result };
	}

	return {};
}


DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExampleItemPlugin")
DEFINE_PLUGIN_VERSION("1.0.5")
DEFINE_PLUGIN_EXECUTE(DatafileItemDetour)
// No auto-key function implemented in this example
// DEFINE_PLUGIN_EXECUTE_AUTOKEY(ExamplePluginAutoKeyFunction) // Uncomment and implement if needed
DEFINE_PLUGIN_TABLE_NAME(L"item")