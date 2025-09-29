#pragma once
#include "DatafileService.h"

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

constexpr auto PLUGIN_API_VERSION = 3;

struct PluginReturnData {
	DrEl* drEl = nullptr;
};

typedef void (*DisplaySystemChatMessageFunc)(const wchar_t*, bool);

struct PluginParamsBase {
	Data::DataManager* dataManager;
};

struct PluginExecuteParams : PluginParamsBase {
	DrMultiKeyTable* table;
	unsigned __int64 key;
	DrEl* (__fastcall* oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key);
	bool isAutoKey;
	DisplaySystemChatMessageFunc displaySystemChatMessage;
};

struct PluginInitParams : PluginParamsBase {
	// Future expansion
};

struct PluginTableHandler {
	const wchar_t* tableName;
	PluginReturnData(*executeFunc)(PluginExecuteParams*);
};

// Function pointer types
typedef PluginReturnData(*PluginExecuteFunc)(PluginExecuteParams*);
typedef const char* (*PluginIdentifierFunc)();
typedef int (*PluginApiVersionFunc)();
typedef const char* (*PluginVersionFunc)();
typedef void (*PluginInitFunc)(PluginInitParams*);

typedef size_t(*PluginTableHandlerCountFunc)();
typedef const PluginTableHandler* (*PluginTableHandlersFunc)();


// Macros to enforce plugin exports
#define DEFINE_PLUGIN_API_VERSION() \
    PLUGIN_EXPORT int PluginApiVersion() { return PLUGIN_API_VERSION; }

#define DEFINE_PLUGIN_IDENTIFIER(name) \
    PLUGIN_EXPORT const char* PluginIdentifier() { return name; }

#define DEFINE_PLUGIN_VERSION(ver) \
    PLUGIN_EXPORT const char* PluginVersion() { return ver; }

#define DEFINE_PLUGIN_INIT(fn) \
	PLUGIN_EXPORT void PluginInit(PluginInitParams* params) { fn(params); }

#define DEFINE_PLUGIN_TABLE_HANDLERS(handlersArray) \
	PLUGIN_EXPORT size_t PluginTableHandlerCount() { return sizeof(handlersArray)/sizeof(handlersArray[0]); } \
	PLUGIN_EXPORT const PluginTableHandler* PluginTableHandlers() { return handlersArray; }