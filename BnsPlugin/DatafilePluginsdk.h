#pragma once
#include "DatafileService.h"

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

constexpr auto PLUGIN_API_VERSION = 2;

struct PluginReturnData {
	DrEl* drEl = nullptr;
};

typedef void (*DisplaySystemChatMessageFunc)(const wchar_t*, bool);

struct PluginParams {
	DrMultiKeyTable* table;
	unsigned __int64 key;
	DrEl* (__fastcall* oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key);
	bool isAutoKey;
	Data::DataManager* dataManager;
	DisplaySystemChatMessageFunc displaySystemChatMessage;
};

// Function pointer types
typedef PluginReturnData(*PluginExecuteFunc)(PluginParams*);
typedef const char* (*PluginIdentifierFunc)();
typedef int (*PluginApiVersionFunc)();
typedef const char* (*PluginVersionFunc)();
typedef const wchar_t* (*PluginTableNameFunc)();

// Macros to enforce plugin exports
#define DEFINE_PLUGIN_API_VERSION() \
    PLUGIN_EXPORT int PluginApiVersion() { return PLUGIN_API_VERSION; }

#define DEFINE_PLUGIN_IDENTIFIER(name) \
    PLUGIN_EXPORT const char* PluginIdentifier() { return name; }

#define DEFINE_PLUGIN_VERSION(ver) \
    PLUGIN_EXPORT const char* PluginVersion() { return ver; }

#define DEFINE_PLUGIN_EXECUTE(fn) \
    PLUGIN_EXPORT PluginReturnData PluginExecute(PluginParams* params) { return fn(params); }

#define DEFINE_PLUGIN_TABLE_NAME(name) \
	PLUGIN_EXPORT const wchar_t* PluginTableName() { return name; }