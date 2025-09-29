#pragma once
#include "DatafileService.h"
#include <cstddef>
#include <type_traits>

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

constexpr int PLUGIN_API_VERSION = 3;

struct PluginReturnData {
	DrEl* drEl = nullptr;
};

using DisplaySystemChatMessageFunc = void(*)(const wchar_t*, bool);

struct PluginParamsBase {
	Data::DataManager* dataManager = nullptr;
};

struct PluginExecuteParams : PluginParamsBase {
	DrMultiKeyTable* table = nullptr;
	unsigned __int64 key = 0;
	DrEl* (__fastcall* oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key) = nullptr;
	bool isAutoKey = false;
	DisplaySystemChatMessageFunc displaySystemChatMessage = nullptr;
};

struct PluginInitParams : PluginParamsBase {
	// Future expansion
};

struct PluginTableHandler {
	const wchar_t* tableName = nullptr;
	PluginReturnData(*executeFunc)(PluginExecuteParams*) = nullptr;
};

// Function pointer types
using PluginExecuteFunc = PluginReturnData(*)(PluginExecuteParams*);
using PluginIdentifierFunc = const char* (*)();
using PluginApiVersionFunc = int (*)();
using PluginVersionFunc = const char* (*)();
using PluginInitFunc = void (*)(PluginInitParams*);
using PluginTableHandlerCountFunc = std::size_t(*)();
using PluginTableHandlersFunc = const PluginTableHandler* (*)();

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
	PLUGIN_EXPORT std::size_t PluginTableHandlerCount() { return std::size(handlersArray); } \
	PLUGIN_EXPORT const PluginTableHandler* PluginTableHandlers() { return handlersArray; }