#include "DatafilePluginsdk.h"

static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
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

static PluginReturnData __fastcall DatafileTextDetour(PluginExecuteParams* params) {
	if (params == nullptr || params->table == nullptr || params->dataManager == nullptr) {
		return {};
	}
	// Example: Return a specific item for a known key
	// Hongmoon's Blessing Recovery Potion -> ncoin test item
	unsigned __int64 key = params->key;

	printf("DatafileTextDetour called with key: %llu\n", key);

	return {};
}

static void __fastcall DatafileItemInit(PluginInitParams* params) {
	// Initialization code if needed
	if (params == nullptr || params->dataManager == nullptr) {
		return;
	}
	// Example: Log initialization
	//params->displaySystemChatMessage(L"ExampleItemPlugin initialized", false);
}

PluginTableHandler handlers[] = {
	{ L"item", &DatafileItemDetour },
	{ L"text", &DatafileTextDetour }
};

DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExampleItemPlugin")
DEFINE_PLUGIN_VERSION("2.0.1")
DEFINE_PLUGIN_INIT(DatafileItemInit)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)