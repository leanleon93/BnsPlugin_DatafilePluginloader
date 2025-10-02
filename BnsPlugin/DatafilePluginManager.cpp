#include "DatafilePluginManager.h"
#include <filesystem>
#include <iostream>
#include <random>
#include <chrono>
#include <string_view>
#include <system_error>
#include "DatafileService.h"
#include "imgui_plugin_api.h"
#include "imgui_manager.h"
#include "imgui.h"

namespace fs = std::filesystem;
static constexpr const char* PLUGINLOADER_IDENTIFIER = "LeanDatafilePluginLoader";

std::unique_ptr<DatafilePluginManager> g_DatafilePluginManager;

#pragma region ImguiWrappers
static void ImGui_TextColored_Wrapper(float r, float g, float b, float a, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	ImGui::TextColoredV(ImVec4(r, g, b, a), fmt, args);
	va_end(args);
}

static bool ImGui_ArrowButton_Wrapper(const char* str_id, int dir) {
	// Clamp dir to valid ImGuiDir values (0-3)
	if (dir < 0) dir = 0;
	if (dir > 3) dir = 3;
	return ImGui::ArrowButton(str_id, static_cast<ImGuiDir>(dir));
}

static bool ImGui_Button_Wrapper(const char* label) {
	return ImGui::Button(label);
}

// InputText: Remove flags/callbacks, just label, buf, and buf_size.
static bool ImGui_InputText_Wrapper(const char* label, char* buf, size_t buf_size) {
	// No extra flags/callbacks; just basic input
	return ImGui::InputText(label, buf, buf_size);
}

// InputInt: Remove extra params, just label and pointer to int.
static bool ImGui_InputInt_Wrapper(const char* label, int* v) {
	return ImGui::InputInt(label, v);
}

// InputFloat: Remove extra params, just label and pointer to float.
static bool ImGui_InputFloat_Wrapper(const char* label, float* v) {
	return ImGui::InputFloat(label, v);
}

// SliderInt: Only label, pointer, min, max
static bool ImGui_SliderInt_Wrapper(const char* label, int* v, int v_min, int v_max) {
	return ImGui::SliderInt(label, v, v_min, v_max);
}

// SliderFloat: Only label, pointer, min, max
static bool ImGui_SliderFloat_Wrapper(const char* label, float* v, float v_min, float v_max) {
	return ImGui::SliderFloat(label, v, v_min, v_max);
}

static bool ImGui_CollapsingHeader_Wrapper(const char* label) {
	return ImGui::CollapsingHeader(label);
}
static void ImGui_BeginChild_Wrapper(const char* str_id) {
	ImGui::BeginChild(str_id);
}

static void ImGui_OpenPopup_Wrapper(const char* str_id) {
	ImGui::OpenPopup(str_id);
}
static bool ImGui_BeginPopup_Wrapper(const char* str_id) {
	return ImGui::BeginPopup(str_id);
}
static bool ImGui_Begin_Wrapper(const char* name, bool* p_open) {
	return ImGui::Begin(name, p_open);
}
static void ImGui_Dummy_Wrapper(float w, float h) {
	ImGui::Dummy(ImVec2(w, h));
}
PluginImGuiAPI g_imguiApi = {
	&ImGui::Text,
	&ImGui_TextColored_Wrapper,
	&ImGui::Separator,
	&ImGui::SameLine,
	&ImGui_Button_Wrapper,
	&ImGui::SmallButton,
	&ImGui_ArrowButton_Wrapper,
	&ImGui::Checkbox,
	&ImGui::RadioButton,
	&ImGui::RadioButton,
	&ImGui_InputText_Wrapper,
	&ImGui_InputInt_Wrapper,
	&ImGui_InputFloat_Wrapper,
	&ImGui_SliderInt_Wrapper,
	&ImGui_SliderFloat_Wrapper,
	&ImGui::Combo,
	&ImGui_CollapsingHeader_Wrapper,
	&ImGui::TreeNode,
	&ImGui::TreePop,
	&ImGui_BeginChild_Wrapper,
	&ImGui::EndChild,
	&ImGui_OpenPopup_Wrapper,
	&ImGui_BeginPopup_Wrapper,
	&ImGui::EndPopup,
	&ImGui::BeginTooltip,
	&ImGui::EndTooltip,
	&ImGui::SetTooltip,
	&ImGui_Begin_Wrapper,
	&ImGui::End,
	&ImGui::Spacing,
	&ImGui_Dummy_Wrapper,
	&ImGui::Indent,
	&ImGui::Unindent,
	&ImGui::PushID,
	&ImGui::PushID,
	&ImGui::PopID
};
#pragma endregion

// Platform-specific hidden attribute helper
#ifdef _WIN32
#include <windows.h>
#include <unordered_set>
#include "BSFunctions.h"
#include <codecvt>
static void set_hidden_attribute(const std::string& path) {
	SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY);
}
#else
static void set_hidden_attribute(const std::string&) {}
#endif

void DatafilePluginManager::ensure_shadow_dir() const {
	std::error_code ec;
	fs::create_directories(_shadow_dir_path, ec);
	if (!ec) set_hidden_attribute(_shadow_dir_path);
}

std::string DatafilePluginManager::get_temp_shadow_dir(const std::string& app_name) {
	auto tmp = fs::temp_directory_path();
	// use unique_path for strong uniqueness instead of rand()
	fs::path p = tmp / fs::path(app_name + "_shadow");
	return p.string();
}

std::string DatafilePluginManager::CopyToShadow(std::string_view plugin_path) const {
	ensure_shadow_dir();
	fs::path src(plugin_path);
	std::string filename = src.stem().string();
	// unique shadow file name (filename + pid + timestamp + random)
	auto now = std::chrono::steady_clock::now().time_since_epoch().count();
	std::random_device rd;
	std::mt19937_64 gen(rd());
	uint64_t r = gen();
	fs::path shadow = fs::path(_shadow_dir_path) / fs::path(filename + "_" + std::to_string(now) + "_" + std::to_string(r) + src.extension().string());
	std::error_code ec;
	fs::copy_file(src, shadow, fs::copy_options::overwrite_existing, ec);
	if (ec) {
		std::cerr << "Failed to copy plugin to shadow: " << src << " -> " << shadow << " : " << ec.message() << '\n';
		return {};
	}
	return shadow.string();
}
extern bool do_reload;

static void GlobalConfigUiPanel(void* userData) {
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	auto stateTexts = g_DatafilePluginManager->GetPluginStateText();
	for (const auto& line : stateTexts) {
		if (line.find("[Failed]") == 0) {
			ImGui_TextColored_Wrapper(1.0f, 0.2f, 0.2f, 1.0f, "%s", line.c_str()); // Red
		}
		else if (line.find("[Loaded]") == 0) {
			ImGui_TextColored_Wrapper(0.2f, 1.0f, 0.2f, 1.0f, "%s", line.c_str()); // Green
		}
		else {
			ImGui::Text("%s", line.c_str());
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Reload all plugins")) {
		do_reload = true;
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Use this button to reload plugins without restarting the game.");
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
}

DatafilePluginManager::DatafilePluginManager(const std::string& folder)
	: _plugins_folder(folder), _shadow_dir_path(get_temp_shadow_dir(PLUGINLOADER_IDENTIFIER)) {
	if (!fs::exists(_plugins_folder) || !fs::is_directory(_plugins_folder)) {
		throw std::runtime_error("Plugins folder does not exist or is not a directory.");
	}
	// clear shadow directory if it exists and is not empty
	if (fs::exists(_shadow_dir_path) && fs::is_directory(_shadow_dir_path)) {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path)) {
			std::error_code ec; fs::remove(entry.path(), ec);
		}
	}
	ImGuiPanelDesc desc = { "Plugin states", GlobalConfigUiPanel, nullptr };
	_panelHandle = RegisterImGuiPanel(&desc);
	ReloadAll();
}

DatafilePluginManager::~DatafilePluginManager() {
	UnloadPlugins();
	if (_panelHandle) {
		UnregisterImGuiPanel(_panelHandle);
		_panelHandle = 0;
	}
}

void DatafilePluginManager::UnloadPlugins() {
	for (auto& kv : _plugins) {
		PluginHandle* handle = kv.second.get();
		if (handle && handle->dll) {
			if (handle->unregister) {
				try {
					handle->unregister();
				}
				catch (...) {
					// ignore
				}
			}
			FreeLibrary(handle->dll);
		}
		if (handle && !handle->shadow_path.empty()) {
			std::error_code ec; fs::remove(handle->shadow_path, ec);
		}
	}
	_table_compare_cache.clear();
	_table_plugin_cache.clear();
	_plugins.clear();

	std::error_code ec;
	if (fs::exists(_shadow_dir_path)) {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path, ec)) {
			std::error_code ec2; fs::remove(entry.path(), ec2);
		}
		fs::remove(_shadow_dir_path, ec);
	}
}

std::vector<std::string> DatafilePluginManager::ReloadAll() {
	std::vector<std::string> results;
	// Remove all plugin handles so they will be forcibly loaded next time
	UnloadPlugins();

	for (const auto& entry : fs::directory_iterator(_plugins_folder)) {
		if (entry.path().extension() == ".dll") {
			if (auto res = ReloadPluginIfChanged(entry.path().string()); !res.empty()) {
				results.push_back(std::move(res));
			}
		}
	}
	return results;
}

std::vector<std::string> DatafilePluginManager::GetPluginStateText()
{
	std::vector<std::string> results;
	for (const auto& kv : _plugins) {
		const auto* handle = kv.second.get();
		if (!handle) continue;
		std::string line;
		if (handle->load_failed) {
			line = "[Failed] ";
			if (handle->identifier) {
				line += handle->identifier();
			}
			else {
				line += kv.first + " (unknown plugin)";
			}
			line += " - " + handle->fail_reason;
		}
		else if (handle->dll && handle->identifier && handle->version) {
			line = std::string("[Loaded] ") + handle->identifier() + " v" + handle->version();
			if (handle->tableHandlers.empty()) {
				line += " (no handlers registered)";
			}
			else {
				line += " (handlers: ";
				for (size_t i = 0; i < handle->tableHandlers.size(); ++i) {
					const auto* th = handle->tableHandlers[i];
					if (th && th->tableName) {
						line += std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(th->tableName);
					}
					else {
						line += "(unknown)";
					}
					if (i + 1 < handle->tableHandlers.size()) line += ", ";
				}
				line += ")";
			}
		}
		else {
			line = "[Unknown state] " + kv.first;
		}
		results.push_back(std::move(line));
	}
	return results;
}

bool DatafilePluginManager::PluginForTableIsRegistered(const wchar_t* table_name) const {
	if (!table_name) return false;
	std::wstring key(table_name);
	if (auto it = _table_compare_cache.find(key); it != _table_compare_cache.end()) {
		return it->second;
	}
	bool found = false;
	for (const auto& kv : _plugins) {
		const auto* handle = kv.second.get();
		if (!handle || !handle->dll) continue;
		for (const auto* handler : handle->tableHandlers) {
			if (handler && handler->tableName && wcscmp(handler->tableName, table_name) == 0) { found = true; break; }
		}
		if (found) break;
	}
	_table_compare_cache.emplace(std::move(key), found);
	return found;
}

std::string DatafilePluginManager::ReloadPluginIfChanged(std::string_view plugin_path_sv) {
	std::string plugin_path(plugin_path_sv);
	fs::path orig_path(plugin_path);
	std::string result;
	if (!fs::exists(orig_path)) {
		std::cerr << "Plugin file does not exist: " << plugin_path << std::endl;
		return result;
	}
	auto current_write_time = fs::last_write_time(orig_path);

	PluginHandle* handle = nullptr;
	if (auto it = _plugins.find(plugin_path); it != _plugins.end()) {
		handle = it->second.get();
	}
	else {
		handle = (_plugins[plugin_path] = std::make_unique<PluginHandle>()).get();
	}

	// If previous load failed and file hasn't changed, skip reload attempt
	if (handle->load_failed && handle->last_write_time == current_write_time) return result;
	// Only reload if not loaded or write_time changed or previous load failed
	if (handle->dll && handle->last_write_time == current_write_time && !handle->load_failed) return result;

	// Unload old DLL & cleanup
	if (handle->dll) {
		for (const auto* handlerPtr : handle->tableHandlers) {
			if (!handlerPtr || !handlerPtr->tableName) continue;
			std::wstring tname(handlerPtr->tableName);
			if (auto it = _table_plugin_cache.find(tname); it != _table_plugin_cache.end()) {
				auto& vec = it->second;
				vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const auto& p) { return p.first == handle && p.second == handlerPtr; }), vec.end());
				if (vec.empty()) _table_plugin_cache.erase(it);
			}
		}
		if (handle->unregister) {
			try {
				handle->unregister();
			}
			catch (...) {
				// ignore
			}
		}
		FreeLibrary(handle->dll);
	}
	if (!handle->shadow_path.empty()) { std::error_code ec; fs::remove(handle->shadow_path, ec); }
	// reset (preserve pointer stability)
	*handle = PluginHandle{};

	std::string shadow_path = CopyToShadow(plugin_path);
	if (shadow_path.empty()) {
		handle->last_write_time = current_write_time;
		handle->load_failed = true;
		handle->fail_reason = "Shadow copy failed";
		return result;
	}

	HMODULE dll = LoadLibraryA(shadow_path.c_str());
	if (!dll) {
		handle->last_write_time = current_write_time;
		handle->load_failed = true;
		handle->fail_reason = "LoadLibrary failed";
		std::cerr << "Failed to load: " << plugin_path << "\n";
		std::error_code ec; fs::remove(shadow_path, ec);
		return result;
	}

	auto init = reinterpret_cast<PluginInitFunc>(GetProcAddress(dll, "PluginInit"));
	auto unregister = reinterpret_cast<PluginUnregisterFunc>(GetProcAddress(dll, "PluginUnregister"));
	auto identifier = reinterpret_cast<PluginIdentifierFunc>(GetProcAddress(dll, "PluginIdentifier"));
	auto api_version = reinterpret_cast<PluginApiVersionFunc>(GetProcAddress(dll, "PluginApiVersion"));
	auto plugin_version = reinterpret_cast<PluginVersionFunc>(GetProcAddress(dll, "PluginVersion"));
	auto tableHandlersFunc = reinterpret_cast<PluginTableHandlersFunc>(GetProcAddress(dll, "PluginTableHandlers"));
	auto tableHandlerCountFunc = reinterpret_cast<PluginTableHandlerCountFunc>(GetProcAddress(dll, "PluginTableHandlerCount"));

	if (!(identifier && api_version && plugin_version && tableHandlersFunc && tableHandlerCountFunc)) {
		std::cerr << "Error: Missing required exports in " << plugin_path << "\n";
		FreeLibrary(dll);
		std::error_code ec; fs::remove(shadow_path, ec);
		handle->last_write_time = current_write_time;
		handle->load_failed = true;
		handle->fail_reason = "Missing required exports";
		return result;
	}

	int reported_api_version = api_version();
	if (reported_api_version != PLUGIN_API_VERSION) {
		std::cerr << "Skipped: " << identifier() << " (incompatible API version " << reported_api_version << ", required " << PLUGIN_API_VERSION << ")\n";
		result = "Incompatible API version: " + std::string(identifier()) + " v" + plugin_version();
		FreeLibrary(dll);
		std::error_code ec; fs::remove(shadow_path, ec);
		handle->last_write_time = current_write_time;
		handle->load_failed = true;
		handle->fail_reason = "Incompatible API version";
		return result;
	}

	// success path
	handle->dll = dll;
	size_t count = tableHandlerCountFunc();
	const PluginTableHandler* handlers = tableHandlersFunc();
	if (handlers && count > 0) {
		handle->tableHandlers.clear();
		handle->tableHandlers.reserve(count);
		for (size_t i = 0; i < count; ++i) handle->tableHandlers.push_back(&handlers[i]);
	}
	handle->init = init;
	handle->unregister = unregister;
	handle->identifier = identifier;
	handle->version = plugin_version;
	handle->last_write_time = current_write_time;
	handle->shadow_path = shadow_path;
	std::cout << "Loaded: " << identifier() << " (API v" << reported_api_version << ", Plugin v" << plugin_version() << ")\n";
	// Call init if available
	if (handle->init && handle->unregister) {
		PluginInitParams params = {};
		params.registerImGuiPanel = &RegisterImGuiPanel;
		params.unregisterImGuiPanel = &UnregisterImGuiPanel;
		params.imgui = &g_imguiApi;
		handle->init(&params);
	}
	result = std::string(handle->identifier()) + " v" + handle->version();
	return result;
}

DrEl* DatafilePluginManager::ExecuteAll(PluginExecuteParams* params) {
	if (!params || !params->table || !params->table->_tabledef || !params->table->_tabledef->name) return nullptr;
	if (!PluginForTableIsRegistered(params->table->_tabledef->name)) return nullptr;

	std::wstring key(params->table->_tabledef->name);
	auto it = _table_plugin_cache.find(key);
	if (it == _table_plugin_cache.end()) {
		std::vector<std::pair<PluginHandle*, const PluginTableHandler*>> handles;
		for (auto& kv : _plugins) {
			PluginHandle* ph = kv.second.get();
			if (!ph || !ph->dll) continue;
			for (const auto* handler : ph->tableHandlers) {
				if (handler && handler->tableName && wcscmp(handler->tableName, params->table->_tabledef->name) == 0) {
					handles.emplace_back(ph, handler);
				}
			}
		}
		it = _table_plugin_cache.emplace(key, std::move(handles)).first;
	}

	for (const auto& pluginTuple : it->second) {
		const auto* handler = pluginTuple.second;
		if (handler && handler->executeFunc) {
			try {
				auto pluginReturnValue = handler->executeFunc(params);
				if (pluginReturnValue.drEl) return pluginReturnValue.drEl;
			}
			catch (const std::exception& ex) {
#ifdef _DEBUG
				std::cerr << "Exception in datafile plugin " << (pluginTuple.first->identifier ? pluginTuple.first->identifier() : "unknown") << ": " << ex.what() << '\n';
#endif
			}
			catch (...) {
#ifdef _DEBUG
				std::cerr << "Unknown exception in datafile plugin " << (pluginTuple.first->identifier ? pluginTuple.first->identifier() : "unknown") << '\n';
#endif
			}
		}
	}
	return nullptr;
}