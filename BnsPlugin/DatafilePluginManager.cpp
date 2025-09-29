#include "DatafilePluginManager.h"
#include <filesystem>
#include <iostream>
#include <random>
#include "DatafileService.h"

namespace fs = std::filesystem;
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
	//clear shadow directory if it exists and is not empty
	if (fs::exists(_shadow_dir_path) && fs::is_directory(_shadow_dir_path)) {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path)) {
			fs::remove(entry.path());
		}
	}
	ReloadAll();
}

DatafilePluginManager::~DatafilePluginManager() {
	UnloadPlugins();
}

void DatafilePluginManager::UnloadPlugins() {
	for (auto& [original, handle] : _plugins) {
		if (handle.dll) FreeLibrary(handle.dll);
		if (!handle.shadow_path.empty()) fs::remove(handle.shadow_path);
	}
	_table_compare_cache.clear();
	_table_plugin_cache.clear();
	_plugins.clear();

	try {
		for (const auto& entry : fs::directory_iterator(_shadow_dir_path))
			fs::remove(entry.path());
		fs::remove(_shadow_dir_path);
	}
	catch (...) {}
}

std::vector<std::string> DatafilePluginManager::ReloadAll() {
	std::vector<std::string> results;
	// Remove all plugin handles so they will be forcibly loaded next time
	UnloadPlugins();

	for (const auto& entry : fs::directory_iterator(_plugins_folder)) {
		if (entry.path().extension() == ".dll") {
			auto res = ReloadPluginIfChanged(entry.path().string());
			if (!res.empty()) {
				results.push_back(res);
			}
		}
	}
	return results;
}

bool DatafilePluginManager::PluginForTableIsRegistered(const wchar_t* table_name) const {
	// Use std::wstring as the cache key
	std::wstring key(table_name ? table_name : L"");
	auto it = _table_compare_cache.find(key);
	if (it != _table_compare_cache.end()) {
		return it->second;
	}
	for (const auto& [original, handle] : _plugins) {
		if (handle.dll) {
			for (const auto* handler : handle.tableHandlers) {
				if (handler->tableName && table_name && wcscmp(handler->tableName, table_name) == 0) {
					_table_compare_cache[key] = true;
					return true;
				}
			}
		}
	}
	_table_compare_cache[key] = false;
	return false;
}

std::string DatafilePluginManager::ReloadPluginIfChanged(const std::string& plugin_path) {
	std::string result;
	fs::path orig_path(plugin_path);
	if (!fs::exists(orig_path)) {
		std::cerr << "Plugin file does not exist: " << plugin_path << std::endl;
		return result;
	}
	auto current_write_time = fs::last_write_time(orig_path);
	auto& handle = _plugins[plugin_path];

	// If previous load failed and file hasn't changed, skip reload attempt
	if (handle.load_failed && handle.last_write_time == current_write_time) {
		return result;
	}
	// Only reload if not loaded or write_time changed or previous load failed
	if (handle.dll && handle.last_write_time == current_write_time && !handle.load_failed) {
		return result;
	}
	// Unload old DLL & cleanup
	if (handle.dll) {
		//remove from table cache
		for (const auto* handlerPtr : handle.tableHandlers) {
			if (!handlerPtr || !handlerPtr->tableName) continue;
			std::wstring tname(handlerPtr->tableName);
			auto it = _table_plugin_cache.find(tname);
			if (it != _table_plugin_cache.end()) {
				auto& vec = it->second;
				vec.erase(
					std::remove_if(
						vec.begin(), vec.end(),
						[&](const std::pair<PluginHandle*, const PluginTableHandler*>& p) {
							return p.first == &handle && p.second == handlerPtr;
						}
					),
					vec.end()
				);
				if (vec.empty()) {
					_table_plugin_cache.erase(it);
				}
			}
		}
		FreeLibrary(handle.dll);
	}
	if (!handle.shadow_path.empty()) fs::remove(handle.shadow_path);
	handle = PluginHandle{};

	std::string shadow_path = CopyToShadow(plugin_path);
	HMODULE dll = LoadLibraryA(shadow_path.c_str());
	if (dll) {
		auto init = (PluginInitFunc)GetProcAddress(dll, "PluginInit");
		auto identifier = (PluginIdentifierFunc)GetProcAddress(dll, "PluginIdentifier");
		auto api_version = (PluginApiVersionFunc)GetProcAddress(dll, "PluginApiVersion");
		auto plugin_version = (PluginVersionFunc)GetProcAddress(dll, "PluginVersion");
		auto tableHandlersFunc = (PluginTableHandlersFunc)GetProcAddress(dll, "PluginTableHandlers");
		auto tableHandlerCountFunc = (PluginTableHandlerCountFunc)GetProcAddress(dll, "PluginTableHandlerCount");
		if (init && identifier && api_version && plugin_version && tableHandlersFunc && tableHandlerCountFunc) {
			int reported_api_version = api_version();
			if (reported_api_version == PLUGIN_API_VERSION) {
				handle.dll = dll;
				size_t count = tableHandlerCountFunc();
				const PluginTableHandler* handlers = tableHandlersFunc();
				if (handlers && count > 0) {
					handle.tableHandlers.clear();
					for (size_t i = 0; i < count; ++i) {
						handle.tableHandlers.push_back(&handlers[i]);
					}
				}
				handle.init = init;
				handle.identifier = identifier;
				handle.version = plugin_version;
				handle.last_write_time = current_write_time;
				handle.shadow_path = shadow_path;
				std::cout << "Loaded: " << identifier()
					<< " (API v" << reported_api_version
					<< ", Plugin v" << plugin_version() << ")\n";
				result = std::string(handle.identifier()) + " v" + handle.version();
			}
			else {
				handle.load_failed = true;
				handle.fail_reason = "Incompatible API version";
				std::cerr << "Skipped: " << identifier()
					<< " (incompatible API version " << reported_api_version
					<< ", required " << PLUGIN_API_VERSION << ")\n";
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
	return result;
}

DrEl* DatafilePluginManager::ExecuteAll(PluginExecuteParams* params) {
	if (params == nullptr)
		return nullptr;
	if (params != nullptr && !PluginForTableIsRegistered(params->table->_tabledef->name)) {
		return nullptr;
	}

	if (params != nullptr) {
		std::wstring key(params->table->_tabledef->name ? params->table->_tabledef->name : L"");
		auto it = _table_plugin_cache.find(key);
		if (it == _table_plugin_cache.end()) {
			std::vector<std::pair<PluginHandle*, const PluginTableHandler*>> handles;
			for (auto& kv : _plugins) {
				if (kv.second.dll && params->table && params->table->_tabledef && params->table->_tabledef->name) {
					for (const auto* handler : kv.second.tableHandlers) {
						if (handler->tableName && wcscmp(handler->tableName, params->table->_tabledef->name) == 0) {
							handles.push_back(std::make_pair(&kv.second, handler));
						}
					}
				}
			}
			_table_plugin_cache[key] = handles;
			it = _table_plugin_cache.find(key);
		}
		for (const auto& pluginTouple : it->second) {
			if (pluginTouple.second->executeFunc) {
				try {
					auto pluginReturnValue = pluginTouple.second->executeFunc(params);
					if (pluginReturnValue.drEl != nullptr)
						return pluginReturnValue.drEl;
				}
				catch (const std::exception& ex) {
#ifdef _DEBUG
					std::cerr << "Exception in datafile plugin " << (pluginTouple.first->identifier ? pluginTouple.first->identifier() : "unknown") << ": " << ex.what() << std::endl;
#endif
				}
				catch (...) {
#ifdef _DEBUG
					std::cerr << "Unknown exception in datafile plugin " << (pluginTouple.first->identifier ? pluginTouple.first->identifier() : "unknown") << std::endl;
#endif
				}
			}
		}
	}

	return nullptr;
}