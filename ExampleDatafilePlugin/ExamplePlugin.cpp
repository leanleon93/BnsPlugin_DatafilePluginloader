#include "DatafilePluginsdk.h"

static PluginReturnData __fastcall DatafileItemDetour(PluginParams* params) {
	if (params == nullptr || params->table == nullptr || params->dataManager == nullptr) {
		return {};
	}
	// Example: Return a specific item for a known key
	// Hongmoon's Blessing Recovery Potion -> ncoin test item
	unsigned __int64 key = params->key;

	if (key == 4295902840) {
		DrEl* result = params->oFind(params->table, 4294967396);
		//params->displaySystemChatMessage(L"ExampleItemPlugin: Redirected item key 4295902840 to 4294967396", false);
		return { result };
	}

	return {};
}


DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExampleItemPlugin")
DEFINE_PLUGIN_VERSION("1.0.8")
DEFINE_PLUGIN_EXECUTE(DatafileItemDetour)
DEFINE_PLUGIN_TABLE_NAME(L"item")