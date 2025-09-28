#include "DatafilePluginManager.h"
#include <filesystem>
#include <iostream>
#include <random>

namespace fs = std::filesystem;
static constexpr int REQUIRED_PLUGIN_API_VERSION = 1;
static constexpr const char* PLUGINLOADER_IDENTIFIER = "LeanDatafilePluginLoader";

DatafilePluginManager g_DatafilePluginManager("datafilePlugins");

// Platform-specific hidden attribute helper
#ifdef _WIN32
#include <windows.h>
#include <unordered_set>
static void set_hidden_attribute(const std::string& path) {
	SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY);
}
#else
static void set_hidden_attribute(const std::string&) {}
#endif

void DatafilePluginManager::ensure_shadow_dir() const {
	fs::create_directories(_shadow_dir_path);
	set_hidden_attribute(_shadow_dir_path);
}

std::string DatafilePluginManager::get_temp_shadow_dir(const std::string& app_name) {
	auto tmp = fs::temp_directory_path();
	return (tmp / (app_name + "_shadow")).string();
}

std::string DatafilePluginManager::CopyToShadow(const std::string& plugin_path) const {
	ensure_shadow_dir();
	std::string filename = fs::path(plugin_path).stem().string();
	std::string shadow_name = filename + "_" + std::to_string(std::rand()) + ".dll";
	std::string shadow_path = _shadow_dir_path + "/" + shadow_name;
	fs::copy_file(plugin_path, shadow_path, fs::copy_options::overwrite_existing);
	return shadow_path;
}

DatafilePluginManager::DatafilePluginManager(const std::string& folder) : _plugins_folder(folder), _shadow_dir_path(get_temp_shadow_dir(PLUGINLOADER_IDENTIFIER)) {
	if (!fs::exists(_plugins_folder) || !fs::is_directory(_plugins_folder)) {
		throw std::runtime_error("Plugins folder does not exist or is not a directory.");
	}
	UnloadPlugins();
	//clear shadow directory if it exists and is not empty
	if (fs::exists(_shadow_dir_path) && fs::is_directory(_shadow_dir_path)) {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path)) {
			fs::remove(entry.path());
		}
	}
	for (const auto& entry : fs::directory_iterator(_plugins_folder)) {
		if (entry.path().extension() == ".dll") {
			ReloadPluginIfChanged(entry.path().string());
		}
	}
}

DatafilePluginManager::~DatafilePluginManager() {
	UnloadPlugins();
}

void DatafilePluginManager::UnloadPlugins() {
	for (auto& [original, handle] : _plugins) {
		if (handle.dll) FreeLibrary(handle.dll);
		if (!handle.shadow_path.empty()) fs::remove(handle.shadow_path);
	}
	_plugins.clear();

	try {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path))
			fs::remove(entry.path());
		fs::remove(_shadow_dir_path);
	}
	catch (...) {}
}

void DatafilePluginManager::SetReloadCheckEnabled(bool enabled) {
	_reload_check_enabled = enabled;
}

bool DatafilePluginManager::IsReloadCheckEnabled() const {
	return _reload_check_enabled;
}

void DatafilePluginManager::ForceReload(const std::string& plugin_path) {
	// Remove any plugin handle so it will be forcibly loaded next time
	auto it = _plugins.find(plugin_path);
	if (it != _plugins.end()) {
		if (it->second.dll) FreeLibrary(it->second.dll);
		if (!it->second.shadow_path.empty()) fs::remove(it->second.shadow_path);
		_plugins.erase(it);
	}
	bool was_enabled = _reload_check_enabled;
	SetReloadCheckEnabled(true);
	ReloadPluginIfChanged(plugin_path);
	SetReloadCheckEnabled(was_enabled);
}

void DatafilePluginManager::ForceReloadAll() {
	// Remove all plugin handles so they will be forcibly loaded next time
	for (auto& [original, handle] : _plugins) {
		if (handle.dll) FreeLibrary(handle.dll);
		if (!handle.shadow_path.empty()) fs::remove(handle.shadow_path);
	}
	_plugins.clear();
	bool was_enabled = _reload_check_enabled;
	SetReloadCheckEnabled(true);
	for (const auto& entry : fs::directory_iterator(_plugins_folder)) {
		if (entry.path().extension() == ".dll") {
			ReloadPluginIfChanged(entry.path().string());
		}
	}
	SetReloadCheckEnabled(was_enabled);
}

bool DatafilePluginManager::PluginForTableIsRegistered(const wchar_t* table_name) const {
	for (const auto& [original, handle] : _plugins) {
		if (handle.dll && handle.tableName) {
			if (wcscmp(handle.tableName(), table_name) == 0) {
				return true;
			}
		}
	}
	return false;
}

void DatafilePluginManager::ReloadPluginIfChanged(const std::string& plugin_path) {
	// === FEATURE TOGGLE: If reload check is disabled, do NOT re-/load plugins ===
	if (!_reload_check_enabled) {
		return;
	}
	fs::path orig_path(plugin_path);
	if (!fs::exists(orig_path)) {
		std::cerr << "Plugin file does not exist: " << plugin_path << std::endl;
		return;
	}
	auto current_write_time = fs::last_write_time(orig_path);
	auto& handle = _plugins[plugin_path];

	// If previous load failed and file hasn't changed, skip reload attempt
	if (handle.load_failed && handle.last_write_time == current_write_time) {
		return;
	}
	// Only reload if not loaded or write_time changed or previous load failed
	if (handle.dll && handle.last_write_time == current_write_time && !handle.load_failed) {
		return;
	}
	// Unload old DLL & cleanup
	if (handle.dll) FreeLibrary(handle.dll);
	if (!handle.shadow_path.empty()) fs::remove(handle.shadow_path);
	handle = PluginHandle{};


	// LOAD LOGIC (runs if reload_check_enabled_ is true and reload needed,
	// or if reload_check_enabled_ is false and plugin not already loaded)
	std::string shadow_path = CopyToShadow(plugin_path);
	HMODULE dll = LoadLibraryA(shadow_path.c_str());
	if (dll) {
		auto execute = (PluginExecuteFunc)GetProcAddress(dll, "PluginExecute");
		auto executeAutokey = (PluginExecuteAutoKeyFunc)GetProcAddress(dll, "PluginExecuteAutoKey");
		auto identifier = (PluginIdentifierFunc)GetProcAddress(dll, "PluginIdentifier");
		auto api_version = (PluginApiVersionFunc)GetProcAddress(dll, "PluginApiVersion");
		auto plugin_version = (PluginVersionFunc)GetProcAddress(dll, "PluginVersion");
		auto table_name = (PluginTableNameFunc)GetProcAddress(dll, "PluginTableName");
		if ((execute || executeAutokey) && identifier && api_version && plugin_version && table_name) {
			int reported_api_version = api_version();
			if (reported_api_version == REQUIRED_PLUGIN_API_VERSION) {
				handle.dll = dll;
				if (execute)
					handle.execute = execute;
				if (executeAutokey)
					handle.executeAutokey = executeAutokey;
				handle.tableName = table_name;
				handle.identifier = identifier;
				handle.version = plugin_version;
				handle.last_write_time = current_write_time;
				handle.shadow_path = shadow_path;
				std::cout << "Loaded: " << identifier()
					<< " (API v" << reported_api_version
					<< ", Plugin v" << plugin_version() << ")\n";
			}
			else {
				handle.load_failed = true;
				handle.fail_reason = "Incompatible API version";
				std::cerr << "Skipped: " << identifier()
					<< " (incompatible API version " << reported_api_version
					<< ", required " << REQUIRED_PLUGIN_API_VERSION << ")\n";
				FreeLibrary(dll);
				fs::remove(shadow_path);
				handle = PluginHandle{ .last_write_time = current_write_time, .load_failed = true, .fail_reason = handle.fail_reason };
			}
		}
		else {
			handle.load_failed = true;
			handle.fail_reason = "Missing required exports";
			std::cerr << "Error: Missing required exports in " << plugin_path << "\n";
			FreeLibrary(dll);
			fs::remove(shadow_path);
			handle = PluginHandle{ .last_write_time = current_write_time, .load_failed = true, .fail_reason = handle.fail_reason };
		}
	}
	else {
		handle.load_failed = true;
		handle.fail_reason = "LoadLibrary failed";
		std::cerr << "Failed to load: " << plugin_path << "\n";
		if (fs::exists(shadow_path)) fs::remove(shadow_path);
		handle = PluginHandle{ .last_write_time = current_write_time, .load_failed = true, .fail_reason = handle.fail_reason };
	}
}

DrEl* DatafilePluginManager::ExecuteAll(PluginParams* params, PluginParamsAutoKey* paramsAutoKey) {
	if (params == nullptr && paramsAutoKey == nullptr)
		return nullptr;
	if (params != nullptr && !PluginForTableIsRegistered(params->table->_tabledef->name)) {
		return nullptr;
	}
	if (paramsAutoKey != nullptr && !PluginForTableIsRegistered(paramsAutoKey->table->_tabledef->name)) {
		return nullptr;
	}
	fs::path plugin_dir(_plugins_folder);
	/*std::cout << "Plugin directory (absolute): " << fs::absolute(plugin_dir) << "\n";*/
	if (!fs::exists(plugin_dir) || !fs::is_directory(plugin_dir)) {
		std::cerr << "Error: Plugin directory does not exist or is not a directory.\n";
		return nullptr;
	}

	// Unload plugins whose original DLL is no longer present
	std::unordered_set<std::string> current_plugins;
	for (const auto& entry : fs::directory_iterator(plugin_dir)) {
		if (entry.path().extension() == ".dll") {
			current_plugins.insert(entry.path().string());
		}
	}
	for (auto it = _plugins.begin(); it != _plugins.end(); ) {
		if (current_plugins.find(it->first) == current_plugins.end()) {
			// Plugin file no longer exists, unload it
			if (it->second.dll) FreeLibrary(it->second.dll);
			if (!it->second.shadow_path.empty()) fs::remove(it->second.shadow_path);
			it = _plugins.erase(it);
		}
		else {
			++it;
		}
	}

	for (const auto& entry : fs::directory_iterator(plugin_dir)) {
		if (entry.path().extension() == ".dll") {
			std::string plugin_path = entry.path().string();
			ReloadPluginIfChanged(plugin_path);
			auto& handle = _plugins[plugin_path];
			if (params != nullptr && handle.execute && wcscmp(handle.tableName(), params->table->_tabledef->name) == 0) {
				auto pluginReturnValue = handle.execute(params);
				if (pluginReturnValue.drEl != nullptr)
					return pluginReturnValue.drEl;
			}

			if (paramsAutoKey != nullptr && handle.executeAutokey && wcscmp(handle.tableName(), paramsAutoKey->table->_tabledef->name) == 0)
			{
				auto pluginReturnValue = handle.executeAutokey(paramsAutoKey);
				if (pluginReturnValue.drEl != nullptr)
					return pluginReturnValue.drEl;
			}
		}
	}
	return nullptr;
}