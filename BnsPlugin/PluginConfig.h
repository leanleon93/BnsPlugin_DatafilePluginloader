#pragma once
#include <string>

class PluginConfig {
public:
	PluginConfig();
	void Initialize();
	void ReloadFromConfig();
	bool IsLoaded() const;
	static constexpr const wchar_t* ConfigFileName = L"templateconfig.ini"; // Compile-time constant

	std::wstring ConfigPath;
	int ConfigValue = 0;
private:
	bool Loaded = false;
};

extern PluginConfig g_PluginConfig;