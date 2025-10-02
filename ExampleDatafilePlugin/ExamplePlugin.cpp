#include "DatafilePluginsdk.h"
#include <EU/text/AAA_text_RecordBase.h>
#include <EU/item/AAA_item_RecordBase.h>
#include <EU/BnsTableNames.h>

static int g_panelHandle = 0;
static RegisterImGuiPanelFn g_register = nullptr;
static UnregisterImGuiPanelFn g_unregister = nullptr;
static PluginImGuiAPI* g_imgui = nullptr;
static std::wstring item100Alias = L"";
static std::wstring itemSwapSourceName2 = L"";
static std::wstring itemSwapTargetName2 = L"";

static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
	// Example: Return a specific item for a known key
	// Changes Hongmoon Uniform to Brashk Outfit
	unsigned __int64 key = params->key;

	if (key == 4295877296) {
		DrEl* result = params->oFind(params->table, 4295917336);
		if (result != nullptr && itemSwapTargetName2.empty()) {
			auto itemRecord = (BnsTables::EU::item_Record*)result;
			auto textTable = DataHelper::GetTable(params->dataManager, L"text");
			if (textTable != nullptr) {
				auto textRecord = (BnsTables::EU::text_Record*)params->oFind(static_cast<DrMultiKeyTable*>(textTable), itemRecord->name2.Key);
				if (textRecord != nullptr) {
					itemSwapTargetName2 = textRecord->text.ReadableText;
				}
			}
		}
		if (itemSwapSourceName2.empty()) {
			auto originalItemRecord = (BnsTables::EU::item_Record*)params->oFind(params->table, key);
			auto textTable = DataHelper::GetTable(params->dataManager, L"text");
			if (textTable != nullptr) {
				auto textRecord = (BnsTables::EU::text_Record*)params->oFind(static_cast<DrMultiKeyTable*>(textTable), originalItemRecord->name2.Key);
				if (textRecord != nullptr) {
					itemSwapSourceName2 = textRecord->text.ReadableText;
				}
			}
		}
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

/* Example ImGui panel with various controls and widgets
 * KEEP IN MIND STATIC STATE IS NOT PRESERVED BETWEEN DLL RELOADS
 * STORE REAL CONFIG IN EXTERNAL FILE IF NEEDED */
static void MyTestPanel(void* userData) {
	static int counter = 0;
	static char buffer[256] = { 0 };
	static float f = 0.0f;
	static bool check = false;
	static int radio = 0;
	static int combo_idx = 0;
	static const char* combo_items[] = { "Apple", "Banana", "Cherry" };
	static bool collapsing_open = true;
	static bool window_open = true;

	g_imgui->Text("Hello from example plugin panel!");
	g_imgui->Separator();

	if (!item100Alias.empty()) {
		g_imgui->Text("Item 100 alias: %ls", item100Alias.c_str());
	}
	else {
		g_imgui->Text("Item 100 alias not loaded.");
	}
	g_imgui->Separator();

	if (!itemSwapSourceName2.empty()) {
		g_imgui->Text("Item swap source name: %ls", itemSwapSourceName2.c_str());
	}
	else {
		g_imgui->Text("Item swap source name not loaded.");
	}

	if (!itemSwapTargetName2.empty()) {
		g_imgui->Text("Item swap target name: %ls", itemSwapTargetName2.c_str());
	}
	else {
		g_imgui->Text("Item swap target name not loaded.");
	}
	g_imgui->Separator();

	if (g_imgui->Button("Click Me")) ++counter;
	g_imgui->SameLine(0.0f, -1.0f);
	if (g_imgui->SmallButton("Small")) --counter;
	g_imgui->Text("Button pressed %d times", counter);

	g_imgui->ArrowButton("Left", 0);
	g_imgui->SameLine(0.0f, -1.0f);
	g_imgui->ArrowButton("Right", 1);
	g_imgui->SameLine(0.0f, -1.0f);
	g_imgui->ArrowButton("Up", 2);
	g_imgui->SameLine(0.0f, -1.0f);
	g_imgui->ArrowButton("Down", 3);

	g_imgui->Checkbox("Check me", &check);
	if (g_imgui->RadioButton("Radio A", radio == 0)) {
		radio = 0;
	}
	g_imgui->SameLine(0.0f, -1.0f);
	if (g_imgui->RadioButton("Radio B", radio == 1)) {
		radio = 1;
	}
	g_imgui->SameLine(0.0f, -1.0f);
	g_imgui->RadioButtonInt("Radio C", &radio, 2);

	g_imgui->InputText("Edit Me", buffer, 256);
	g_imgui->InputInt("Integer", &counter);
	g_imgui->SliderInt("Slider", &counter, 0, 200);
	g_imgui->InputFloat("Float", &f);
	g_imgui->SliderFloat("Float Slider", &f, 0.0f, 1.0f);

	g_imgui->Combo("Combo", &combo_idx, combo_items, 3, 3);

	if (g_imgui->CollapsingHeader("Collapsing Header")) {
		g_imgui->Text("Inside collapsing header");
	}

	if (g_imgui->TreeNode("Tree Node")) {
		g_imgui->Text("Inside tree node");
		g_imgui->TreePop();
	}

	g_imgui->BeginChild("ChildWindow");
	g_imgui->Text("Inside child window");
	g_imgui->EndChild();

	/*if (g_imgui->Button("Open Popup")) {
		g_imgui->OpenPopup("MyPopup");
	}
	if (g_imgui->BeginPopup("MyPopup")) {
		g_imgui->Text("Popup content!");
		if (g_imgui->Button("Close")) g_imgui->EndPopup();
		g_imgui->EndPopup();
	}*/

	/*g_imgui->BeginTooltip();
	g_imgui->Text("Tooltip text!");
	g_imgui->EndTooltip();*/

	//g_imgui->SetTooltip("Quick tooltip!");

	/*if (g_imgui->Begin("Extra Window", &window_open)) {
		g_imgui->Text("Inside another window");
		g_imgui->End();
	}*/

	/*g_imgui->Spacing();
	g_imgui->Dummy(10.0f, 10.0f);*/
}

static void __fastcall Init(PluginInitParams* params) {
	if (params && params->registerImGuiPanel && params->unregisterImGuiPanel && params->imgui)
	{
		g_imgui = params->imgui;
		g_register = params->registerImGuiPanel;
		g_unregister = params->unregisterImGuiPanel;
		ImGuiPanelDesc desc = { "ExampleDatafilePlugin Panel", MyTestPanel, nullptr };
		g_panelHandle = g_register(&desc);
	}
	if (params && params->dataManager) {
		auto itemTable = DataHelper::GetTable(params->dataManager, L"item");
		if (itemTable) {
			auto mkTable = static_cast<DrMultiKeyTable*>(itemTable);
			auto item100 = params->oFind(mkTable, 4294967396);
			if (item100) {
				auto itemRecord = (BnsTables::EU::item_Record*)item100;
				item100Alias = std::wstring(itemRecord->alias ? itemRecord->alias : L"(no alias)");
			}
		}
	}
}

static void __fastcall Unregister() {
	if (g_unregister && g_panelHandle != 0) {
		g_unregister(g_panelHandle);
		g_panelHandle = 0;
	}
}

PluginTableHandler handlers[] = {
	{ L"item", &DatafileItemDetour },
	{ L"text", &DatafileTextDetour }
};

DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ExampleDatafilePlugin")
DEFINE_PLUGIN_VERSION("3.1.0")
DEFINE_PLUGIN_INIT(Init, Unregister)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)