#include "DatafilePluginsdk.h"

static PluginReturnData __fastcall ExamplePluginFunction(PluginParams* params) {
	if (params == nullptr || params->table == nullptr || params->dataManager == nullptr) {
		return {};
	}
	// Example: Log the key being looked up
	unsigned __int64 key = params->key;
	//printf("ExamplePlugin: Looking up key %llu\n", key);
	// Call the original function to get the DrEl
	// DrEl* result = params->oFind_b8(params->table, key);
	return {};
}


DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExamplePlugin")
DEFINE_PLUGIN_VERSION("1.0.1")
DEFINE_PLUGIN_EXECUTE(ExamplePluginFunction)
// No auto-key function implemented in this example
// DEFINE_PLUGIN_EXECUTE_AUTOKEY(ExamplePluginAutoKeyFunction) // Uncomment and implement if needed
DEFINE_PLUGIN_TABLE_NAME(L"item")