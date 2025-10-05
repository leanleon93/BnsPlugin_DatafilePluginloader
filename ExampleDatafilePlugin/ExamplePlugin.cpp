#include "DatafilePluginsdk.h"
#include <EU/text/AAA_text_RecordBase.h>
#include <EU/item/AAA_item_RecordBase.h>
#include <EU/BnsTableNames.h>
#include <algorithm>

static int g_panelHandle = 0;
static RegisterImGuiPanelFn g_register = nullptr;
static UnregisterImGuiPanelFn g_unregister = nullptr;
static PluginImGuiAPI* g_imgui = nullptr;
static std::wstring item100Alias = L"";
static std::wstring itemSwapSourceName2 = L"";
static std::wstring itemSwapTargetName2 = L"";
static int itemCount = 0;
static int textCount = 0;
static std::vector<std::tuple<int, signed char, std::wstring>> itemList;
static std::vector<std::tuple<int, signed char, std::wstring, std::wstring>> itemList2;
static std::vector<std::tuple<unsigned __int64, std::wstring, std::wstring>> textList;
static bool imguiDemo = false;

static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
	// Example: Return a specific item for a known key
	// Changes Hongmoon Uniform to Brashk Outfit
	unsigned __int64 key = params->key;

	if (key == 4295877296) {
		DrEl* result = params->oFind(params->table, 4295917336);
		if (result != nullptr && itemSwapTargetName2.empty()) {
			auto itemRecord = (BnsTables::EU::item_Record*)result;
			auto textRecord = GetRecord<BnsTables::EU::text_Record>(params->dataManager, L"text", itemRecord->name2.Key, params->oFind);
			if (textRecord != nullptr) {
				itemSwapTargetName2 = textRecord->text.ReadableText;
			}
		}
		if (itemSwapSourceName2.empty()) {
			auto originalItemRecord = (BnsTables::EU::item_Record*)params->oFind(params->table, key);
			auto textRecord = GetRecord<BnsTables::EU::text_Record>(params->dataManager, L"text", originalItemRecord->name2.Key, params->oFind);
			if (textRecord != nullptr) {
				itemSwapSourceName2 = textRecord->text.ReadableText;
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

	g_imgui->Text("Total item records: %d", itemCount);
	g_imgui->Separator();

	//sort itemList by id
	std::sort(itemList.begin(), itemList.end(), [](const auto& a, const auto& b) {
		if (std::get<0>(a) == std::get<0>(b)) {
			return std::get<1>(a) < std::get<1>(b);
		}
		return std::get<0>(a) < std::get<0>(b);
		});
	g_imgui->Text("First 10 items:");
	for (size_t i = 0; i < itemList.size() && i < 10; i++) {
		auto& [id, level, alias] = itemList[i];
		g_imgui->Text("ID: %d, Level: %d, Alias: %ls", id, level, alias.c_str());
	}
	g_imgui->Separator();

	g_imgui->Text("First 10 items with id >= 10000:");
	for (size_t i = 0; i < itemList2.size() && i < 10; i++) {
		auto& [id, level, alias, text] = itemList2[i];
		g_imgui->Text("ID: %d, Level: %d, Alias: %ls, Name: %ls", id, level, alias.c_str(), text.c_str());
	}
	g_imgui->Separator();

	g_imgui->Text("Total text records: %d", textCount);
	g_imgui->Text("First 10 texts:");
	for (size_t i = 0; i < textList.size() && i < 10; i++) {
		auto& [autoId, alias, readableText] = textList[i];
		g_imgui->Text("AutoID: %llu, Alias: %ls, Text: %ls", autoId, alias.c_str(), readableText.c_str());
	}
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

	if (imguiDemo) {
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
	}
}

static void __fastcall Init(PluginInitParams* params) {
	if (params && params->registerImGuiPanel && params->unregisterImGuiPanel && params->imgui)
	{
		g_imgui = params->imgui;
		g_register = params->registerImGuiPanel;
		g_unregister = params->unregisterImGuiPanel;
		ImGuiPanelDesc desc = { "ExampleDatafilePlugin Panel", MyTestPanel, nullptr };
		g_panelHandle = g_register(&desc, false);
	}
	if (params && params->dataManager) {
		auto item100Key = BnsTables::EU::item_Record::Key{};
		item100Key.key = 0;
		item100Key.id = 100;
		item100Key.level = 1;

		auto item100 = GetRecord<BnsTables::EU::item_Record>(params->dataManager, L"item", item100Key.key, params->oFind);
		if (item100) {
			auto itemRecord = (BnsTables::EU::item_Record*)item100;
			item100Alias = std::wstring(itemRecord->alias ? itemRecord->alias : L"(no alias)");
		}
		auto itemTable = GetTable(params->dataManager, L"item");
		itemCount = GetRecordCount(params->dataManager, L"item");
		textCount = GetRecordCount(params->dataManager, L"text");
		ForEachRecord<BnsTables::EU::item_Record>(params->dataManager, L"item", [](BnsTables::EU::item_Record* rec, size_t idx) {
			if (rec) {
				itemList.push_back(std::make_tuple(rec->key.id, rec->key.level, rec->alias ? rec->alias : L"(no alias)"));
			}
			return true;
			}, 10);
		int count = 0;
		ForEachRecord<BnsTables::EU::text_Record>(params->dataManager, L"text", [](BnsTables::EU::text_Record* rec, size_t idx) {
			if (rec) {
				textList.push_back(std::make_tuple(rec->key.autoId, rec->alias, rec->text.ReadableText ? rec->text.ReadableText : L"(no text)"));
			}
			return true;
			}, 10);
		auto matches = GetRecordsWhere<BnsTables::EU::item_Record>(params->dataManager, L"item", [](BnsTables::EU::item_Record* rec) {
			return rec && rec->key.id >= 10000;
			}, 10);
		for (auto& rec : matches) {
			if (rec) {
				auto textRecord = GetText<BnsTables::EU::text_Record>(params->dataManager, rec->name2.Key, params->oFind);
				itemList2.push_back(std::make_tuple(rec->key.id, rec->key.level, rec->alias ? rec->alias : L"(no alias)", textRecord ? (textRecord->text.ReadableText ? textRecord->text.ReadableText : L"(no text)") : L"(text not found)"));
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
DEFINE_PLUGIN_VERSION("3.1.1")
DEFINE_PLUGIN_INIT(Init, Unregister)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)