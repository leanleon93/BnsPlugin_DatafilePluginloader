#include "DatafilePluginsdk.h"
#include <EU/text/AAA_text_RecordBase.h>
#include <EU/item/AAA_item_RecordBase.h>
#include <EU/BnsTableNames.h>
#include <algorithm>
#include <EU/quest/AAA_quest_RecordBase.h>
#include <d3d11.h>
#include <imgui_manager.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <deps/xorstr.hpp>



static int g_panelHandle = 0;
static int g_imageOverlayHandle = 0;
static Data::DataManager* g_dataManager = nullptr;
static DrEl* (__fastcall* g_oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key) = nullptr;
static RegisterImGuiPanelFn g_register = nullptr;
static UnregisterImGuiPanelFn g_unregister = nullptr;
static UnregisterDetoursFunc g_unregisterDetours = nullptr;
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
static ID3D11Device* (*g_GetD3DDevice)() = nullptr;
static int my_image_width = 0;
static int my_image_height = 0;
static ID3D11ShaderResourceView* my_image_texture = NULL;
static bool imageShowing = false;
static float imagePosX = 50.0f;
static float imagePosY = 50.0f;

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	if (g_GetD3DDevice() == nullptr) return false;
	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_GetD3DDevice()->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	g_GetD3DDevice()->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	if (g_GetD3DDevice() == nullptr) return false;
	FILE* f = nullptr;
	if (fopen_s(&f, file_name, "rb") != 0 || f == nullptr) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	size_t file_size = (size_t)ftell(f);
	if (file_size == (size_t)-1) {
		fclose(f);
		return false;
	}

	fseek(f, 0, SEEK_SET);
	void* file_data = g_imgui->MemAlloc(file_size);
	fread(file_data, 1, file_size, f);
	fclose(f);

	bool ret = LoadTextureFromMemory(file_data, file_size, out_srv, out_width, out_height);
	g_imgui->MemFree(file_data);
	return ret;
}

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

static void ImageOverlay(void* userData) {
	if (my_image_texture == NULL) {
		if (my_image_texture != NULL) {
			my_image_texture->Release();
			my_image_texture = NULL;
		}
		if (LoadTextureFromFile("datafilePlugins/test.png", &my_image_texture, &my_image_width, &my_image_height)) {
			// Loaded successfully
		}
		else {
			// Failed to load
		}
	}
	if (imageShowing && my_image_texture != NULL) {
		g_imgui->DisplayImageAtPos((void*)my_image_texture, (float)my_image_width, (float)my_image_height, imagePosX, imagePosY, 0.0f, 0.0f, 1.0f, 1.0f);
		float offsetX = imagePosX - (2560 / 2.0f);
		float offsetY = (imagePosY + my_image_height + 10.0f) - (1440 / 2.0f);

		const char* text = "TEST IMAGE TEXT";
		// Call DisplayTextInCenter with adjusted offsets
		float textPosX = imagePosX + (my_image_width / 2.0f) - ((0) / 2.0f);
		float textPosY = imagePosY + my_image_height + 10.0f; // Slightly below the image

		// Display the text
		g_imgui->DisplayTextInCenter(text, 24.0f, 0xFF0000FF, textPosX - (2560 / 2.0f), textPosY - (1440 / 2.0f), true, "");
	}
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

	if (g_imgui->Button("Reload image")) {
		if (my_image_texture != NULL) {
			my_image_texture->Release();
			my_image_texture = NULL;
		}
		if (LoadTextureFromFile("datafilePlugins/test.png", &my_image_texture, &my_image_width, &my_image_height)) {
			// Loaded successfully
		}
		else {
			// Failed to load
		}
	}
	if (g_imgui->Button("Toggle Image")) {
		imageShowing = !imageShowing;
	}
	g_imgui->Text("Image Showing: %s", imageShowing ? "Yes" : "No");
	g_imgui->Text("Image Ready: %s", (my_image_texture != NULL) ? "Yes" : "No");
	g_imgui->SliderFloat("Image Pos X", &imagePosX, 0.0f, 2560.0f);
	g_imgui->SliderFloat("Image Pos Y", &imagePosY, 0.0f, 1440.0f);

	g_imgui->Separator();
	if (g_imgui->Button("Debug Button")) {
		auto quest = GetQuest(g_dataManager, 28337);
		if (quest != nullptr) {
			BnsTables::EU::quest_Record* questRecord = (BnsTables::EU::quest_Record*)quest->_questRecord;
			if (questRecord->name2.Key != 0) {
				auto textRecord = GetRecord<BnsTables::EU::text_Record>(g_dataManager, L"text", questRecord->name2.Key, g_oFind);
				if (textRecord != nullptr) {
					g_imgui->Text("Quest 28337 name: %ls", textRecord->text.ReadableText);
					int debug = 0;
				}
			}
		}
		ForEachQuest(g_dataManager, [&](Quest* quest, size_t idx) {
			if (quest) {
				BnsTables::EU::quest_Record* questRecord = (BnsTables::EU::quest_Record*)quest->_questRecord;
				if (questRecord->key.id == 28337) {
					if (questRecord->name2.Key != 0) {
						auto textRecord = GetRecord<BnsTables::EU::text_Record>(g_dataManager, L"text", questRecord->name2.Key, g_oFind);
						if (textRecord != nullptr) {
							g_imgui->Text("(From ForEachQuest) Quest 28337 name: %ls", textRecord->text.ReadableText);
							int debug = 0;
						}
					}
					return false; // stop iteration
				}
			}
			return true;
			});
	}

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
		ImGuiPanelDesc desc2 = { "ExampleDatafilePlugin Panel", ImageOverlay, nullptr };
		g_imageOverlayHandle = g_register(&desc2, true);
	}
	if (params && params->GetD3DDevice) {
		g_GetD3DDevice = params->GetD3DDevice;
	}
	if (params && params->imgui && params->GetD3DDevice) {
		bool ret = LoadTextureFromFile("datafilePlugins/test.png", &my_image_texture, &my_image_width, &my_image_height);
		assert(ret);
	}
	if (params && params->dataManager) {
		g_dataManager = params->dataManager;
		g_oFind = params->oFind;
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
	if (g_unregister && g_imageOverlayHandle != 0) {
		g_unregister(g_imageOverlayHandle);
		g_imageOverlayHandle = 0;
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