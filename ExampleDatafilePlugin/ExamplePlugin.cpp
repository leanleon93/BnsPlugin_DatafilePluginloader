#include "DatafilePluginsdk.h"
#include <EU/text/AAA_text_RecordBase.h>

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

const std::wstring customText = L"<arg p=\"1:integer\"/> Fake Frames";

//Changes the FPS indicator text to Fake Frames instead
static PluginReturnData __fastcall DatafileTextDetour(PluginExecuteParams* params) {
	if (params == nullptr || params->table == nullptr || params->dataManager == nullptr) {
		return {};
	}

	unsigned __int64 key = params->key;

	if (key == 690565) {
		DrEl* record = params->oFind(params->table, params->key);
		auto textRecord = (BnsTables::EU::text_Record*)record;
		//printf("ExampleItemPlugin: Original text for key 690565: %ls\n", textRecord->text.ReadableText);
		textRecord->text.ReadableText = const_cast<wchar_t*>(customText.c_str());
	}

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
DEFINE_PLUGIN_IDENTIFIER("ExampleDatafilePlugin")
DEFINE_PLUGIN_VERSION("2.0.1")
DEFINE_PLUGIN_INIT(DatafileItemInit)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)