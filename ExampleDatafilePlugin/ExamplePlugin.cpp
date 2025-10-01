#include "DatafilePluginsdk.h"
#include <EU/text/AAA_text_RecordBase.h>
#include <EU/BnsTableNames.h>


static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
	// Example: Return a specific item for a known key
	// Changes Hongmoon Uniform to Brashk Outfit
	unsigned __int64 key = params->key;

	if (key == 4295877296) {
		DrEl* result = params->oFind(params->table, 4295917336);
		//params->displaySystemChatMessage(L"ExampleItemPlugin: Redirected item key 4295902840 to 4294967396", false);
		return { result };
	}

	return {};
}

const std::wstring customText = L"<arg p=\"1:integer\"/> Fake Frames";

//Changes the FPS indicator text to Fake Frames instead
static PluginReturnData __fastcall DatafileTextDetour(PluginExecuteParams* params) {
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
	unsigned __int64 key = params->key;
	//printf("ExampleItemPlugin: Accessing text key %llu\n", key);
	if (key == 690565) {
		DrEl* record = params->oFind(params->table, params->key);
		auto textRecord = (BnsTables::EU::text_Record*)record;
		//printf("ExampleItemPlugin: Original text for key 690565: %ls\n", textRecord->text.ReadableText);
		textRecord->text.ReadableText = const_cast<wchar_t*>(customText.c_str());
	}
	//if (key == 1193257) {
	//	DrEl* record = params->oFind(params->table, params->key);
	//	auto textRecord = (BnsTables::EU::text_Record*)record;
	//	//printf("ExampleItemPlugin: Original text for key 690565: %ls\n", textRecord->text.ReadableText);
	//	textRecord->text.ReadableText = const_cast<wchar_t*>(customText.c_str());
	//}

	return {};
}

static int g_panelHandle = 0;
static UnregisterImGuiPanelFn g_UnregisterPanel = nullptr;
static int counter = 0;

// Dummy ImGui functions for IntelliSense
namespace ImGui {
	inline void Text(const char*, ...) {}
	inline bool Button(const char*, ...) { return false; }
	inline void SameLine() {}
	// Add any other functions you want IntelliSense for.
}

static void RenderPanel(void*) {
	ImGui::Text("Hello from ExamplePlugin!");
	if (ImGui::Button("Click Me")) ++counter;
	ImGui::Text("Button pressed %d times", counter);
}

static void __fastcall Init(PluginInitParams* params) {
	g_UnregisterPanel = params->unregisterImGuiPanel;
	ImGuiPanelDesc desc = { "Plugin Panel", RenderPanel, nullptr };
	g_panelHandle = params->registerImGuiPanel(&desc);
}

PluginTableHandler handlers[] = {
	{ L"item", &DatafileItemDetour },
	{ L"text", &DatafileTextDetour }
};

DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExampleDatafilePlugin")
DEFINE_PLUGIN_VERSION("3.0.0")
DEFINE_PLUGIN_INIT(Init)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)