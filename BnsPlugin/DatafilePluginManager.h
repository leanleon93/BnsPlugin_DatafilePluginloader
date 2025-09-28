#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <Windows.h>
#include "DatafilePluginsdk.h"

struct PluginHandle {
	HMODULE dll = nullptr;
	PluginExecuteFunc execute = nullptr;
	PluginExecuteAutoKeyFunc executeAutokey = nullptr;
	PluginIdentifierFunc identifier = nullptr;
	PluginVersionFunc version = nullptr;
	PluginTableNameFunc tableName = nullptr;
	std::filesystem::file_time_type last_write_time;
	std::string shadow_path;
	bool load_failed = false;
	std::string fail_reason;
};

class DatafilePluginManager {
public:
	explicit DatafilePluginManager(const std::string& folder);
	~DatafilePluginManager();
	DrEl* ExecuteAll(PluginParams* params, PluginParamsAutoKey* paramsAutoKey);
	void UnloadPlugins();

	void ReloadAll();
private:
	std::string _plugins_folder;
	const std::string _shadow_dir_path;
	std::unordered_map<std::string, PluginHandle> _plugins; // key: original dll path
	mutable std::unordered_map<std::wstring, bool> _table_compare_cache;
	mutable std::unordered_map<std::wstring, std::vector<PluginHandle*>> _table_plugin_cache;

	bool PluginForTableIsRegistered(const wchar_t* table_name) const;
	void ReloadPluginIfChanged(const std::string& plugin_path);
	std::string CopyToShadow(const std::string& plugin_path) const;
	void ensure_shadow_dir() const;
	static std::string get_temp_shadow_dir(const std::string& app_name);
};

extern DatafilePluginManager g_DatafilePluginManager;