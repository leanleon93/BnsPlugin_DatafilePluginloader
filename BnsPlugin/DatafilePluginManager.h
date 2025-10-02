#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <memory>
#include <Windows.h>
#include "DatafilePluginsdk.h"

// RAII / data handle for a plugin. Stored behind stable unique_ptr so pointers cached elsewhere remain valid.
struct PluginHandle {
	HMODULE dll = nullptr;
	std::vector<const PluginTableHandler*> tableHandlers; // raw pointers owned by the plugin DLL
	PluginIdentifierFunc identifier = nullptr;
	PluginVersionFunc version = nullptr;
	PluginInitFunc init = nullptr;
	PluginUnregisterFunc unregister = nullptr;
	std::filesystem::file_time_type last_write_time{};
	std::string shadow_path; // copied DLL path (shadow copy)
	bool load_failed = false;
	std::string fail_reason;
};

class DatafilePluginManager {
public:
	explicit DatafilePluginManager(const std::string& folder);
	~DatafilePluginManager();
	DrEl* ExecuteAll(PluginExecuteParams* params);
	void UnloadPlugins();

	[[nodiscard]] std::vector<std::string> ReloadAll();
	std::vector<std::string> GetPluginStateText();
private:
	std::string _plugins_folder; // source folder for plugins
	const std::string _shadow_dir_path; // temp shadow dir where we load from
	// key: original dll path -> stable owning pointer for PluginHandle
	std::unordered_map<std::string, std::unique_ptr<PluginHandle>> _plugins;
	// caches
	mutable std::unordered_map<std::wstring, bool> _table_compare_cache; // table name -> any plugin registered?
	mutable std::unordered_map<std::wstring, std::vector<std::pair<PluginHandle*, const PluginTableHandler*>>> _table_plugin_cache; // table name -> (plugin, handler) list

	[[nodiscard]] bool PluginForTableIsRegistered(const wchar_t* table_name) const;
	[[nodiscard]] std::string ReloadPluginIfChanged(std::string_view plugin_path);
	[[nodiscard]] std::string CopyToShadow(std::string_view plugin_path) const;
	void ensure_shadow_dir() const;
	[[nodiscard]] static std::string get_temp_shadow_dir(const std::string& app_name);
	int _panelHandle = 0;
};

extern std::unique_ptr<DatafilePluginManager> g_DatafilePluginManager;