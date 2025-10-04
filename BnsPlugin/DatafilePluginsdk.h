#pragma once
#include <cstddef>
#include <type_traits>
#include <cwchar>
#include <unordered_map>
#include <functional>
#include <string>
#include "Data.h"
#include "imgui_plugin_api.h"

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

constexpr int PLUGIN_API_VERSION = 9;


struct PluginReturnData {
	DrEl* drEl = nullptr;
};

using DisplaySystemChatMessageFunc = void(*)(const wchar_t*, bool);

struct PluginParamsBase {
	Data::DataManager* dataManager = nullptr;
	DrEl* (__fastcall* oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key) = nullptr;
};

struct PluginExecuteParams : PluginParamsBase {

	DrMultiKeyTable* table = nullptr;
	unsigned __int64 key = 0;
	DisplaySystemChatMessageFunc displaySystemChatMessage = nullptr;
};

struct PluginInitParams : PluginParamsBase {
	RegisterImGuiPanelFn registerImGuiPanel = nullptr;
	UnregisterImGuiPanelFn unregisterImGuiPanel = nullptr;
	PluginImGuiAPI* imgui = nullptr;
};

struct PluginTableHandler {
	const wchar_t* tableName = nullptr;
	PluginReturnData(*executeFunc)(PluginExecuteParams*) = nullptr;
};

struct VersionInfo {
	unsigned int key;
	short major;
	short minor;
};

using OFindFunc = DrEl * (__fastcall*)(DrMultiKeyTable* thisptr, unsigned __int64 key);

inline DrMultiKeyTable* GetTable(const Data::DataManager* dataManager, int tableId) {
	if (dataManager == nullptr) {
		return nullptr;
	}
	auto index = tableId - 1;
	int arraySize = sizeof(dataManager->_loaderDefs) / sizeof(dataManager->_loaderDefs[0]);
	if (index < 0 || index >= arraySize) {
		return nullptr;
	}
	auto loaderDef = dataManager->_loaderDefs[index];
	auto tableDef = loaderDef.tableDef;
	if (tableDef == nullptr) {
		return nullptr;
	}
	auto table = dataManager->_loaderDefs[index].table;
	return static_cast<DrMultiKeyTable*>(table);
}

inline DrMultiKeyTable* GetTable(const Data::DataManager* dataManager, const wchar_t* tableName) {
	if (dataManager == nullptr || tableName == nullptr) {
		return nullptr;
	}
	int arraySize = sizeof(dataManager->_loaderDefs) / sizeof(dataManager->_loaderDefs[0]);
	for (int i = 0; i < arraySize; i++) {
		auto loaderDef = dataManager->_loaderDefs[i];
		auto tableDef = loaderDef.tableDef;
		if (tableDef == nullptr) continue;
		auto name = tableDef->name;
		if (wcscmp(name, tableName) == 0) {
			auto table = dataManager->_loaderDefs[i].table;
			return static_cast<DrMultiKeyTable*>(table);
		}
	}
	return nullptr;
}


inline DrEl* GetRecord(DrMultiKeyTable* table, unsigned __int64 key, OFindFunc oFind) {
	if (table == nullptr || oFind == nullptr) {
		return nullptr;
	}
	return oFind(table, key);
}

template<typename T>
inline T* GetRecord(DrMultiKeyTable* table, unsigned __int64 key, OFindFunc oFind) {
	return (T*)(GetRecord(table, key, oFind));
}

inline DrEl* GetRecord(const Data::DataManager* dataManager, const wchar_t* tableName, unsigned __int64 key, OFindFunc oFind) {
	auto table = GetTable(dataManager, tableName);
	if (table == nullptr) {
		return nullptr;
	}
	return GetRecord(table, key, oFind);
}

template<typename T>
inline T* GetRecord(const Data::DataManager* dataManager, const wchar_t* tableName, unsigned __int64 key, OFindFunc oFind) {
	return (T*)(GetRecord(dataManager, tableName, key, oFind));
}

inline void ForEachRecord(
	DrMultiKeyTable* table,
	std::function<bool(DrEl*, size_t)> func,
	size_t limit = SIZE_MAX
) {
	if (!table || !table->__vftable || !func) return;
	auto innerIter = table->__vftable->createInnerIter(table);
	if (!innerIter) return;
	size_t index = 0;
	do {
		if (!innerIter->_vtptr->IsValid(innerIter)) continue;
		DrEl* el = innerIter->_vtptr->Ptr(innerIter);
		if (el) {
			if (!func(el, index)) break; // break if func returns false
			++index;
			if (index >= limit) break;   // break if limit reached
		}
	} while (innerIter->_vtptr->Next(innerIter));
}

template<typename RecordType>
inline void ForEachRecord(
	DrMultiKeyTable* table,
	std::function<bool(RecordType*, size_t)> func,
	size_t limit = SIZE_MAX
) {
	ForEachRecord(table, [&](DrEl* el, size_t idx) {
		return func((RecordType*)(el), idx);
		}, limit);
}

inline void ForEachRecord(
	const Data::DataManager* dataManager,
	const wchar_t* tableName,
	std::function<bool(DrEl*, size_t)> func,
	size_t limit = SIZE_MAX
) {
	auto table = GetTable(dataManager, tableName);
	ForEachRecord(table, func, limit);
}

template<typename RecordType>
inline void ForEachRecord(
	const Data::DataManager* dataManager,
	const wchar_t* tableName,
	std::function<bool(RecordType*, size_t)> func,
	size_t limit = SIZE_MAX
) {
	ForEachRecord(dataManager, tableName, [&](DrEl* el, size_t idx) {
		return func((RecordType*)(el), idx);
		}, limit);
}

template<typename RecordType>
inline std::vector<RecordType*> GetRecordsWhere(
	DrMultiKeyTable* table,
	std::function<bool(RecordType*)> predicate,
	size_t limit = SIZE_MAX
) {
	std::vector<RecordType*> results;
	ForEachRecord<RecordType>(table, [&](RecordType* el, size_t) {
		if (predicate(el)) {
			results.push_back(el);
			if (results.size() >= limit) return false;
		}
		return true;
		});
	return results;
}

template<typename RecordType>
inline std::vector<RecordType*> GetRecordsWhere(
	const Data::DataManager* dataManager,
	const wchar_t* tableName,
	std::function<bool(RecordType*)> predicate,
	size_t limit = SIZE_MAX
) {
	auto table = GetTable(dataManager, tableName);
	return GetRecordsWhere<RecordType>(table, predicate, limit);
}

template<typename RecordType>
inline RecordType* GetFirstRecordWhere(
	DrMultiKeyTable* table,
	std::function<bool(RecordType*)> predicate
) {
	RecordType* result = nullptr;
	ForEachRecord<RecordType>(table, [&](RecordType* el, size_t) {
		if (predicate(el)) {
			result = el;
			return false; // stop iteration
		}
		return true;
		});
	return result;
}

template<typename RecordType>
inline RecordType* GetFirstRecordWhere(
	const Data::DataManager* dataManager,
	const wchar_t* tableName,
	std::function<bool(RecordType*)> predicate
) {
	auto table = GetTable(dataManager, tableName);
	return GetFirstRecordWhere<RecordType>(table, predicate);
}

template<typename RecordType>
inline RecordType* GetText(
	const Data::DataManager* dataManager,
	unsigned __int64 textKey,
	OFindFunc oFind
) {
	static DrMultiKeyTable* textTable = nullptr;
	if (textTable == nullptr) {
		textTable = GetTable(dataManager, L"text");
	}
	return GetRecord<RecordType>(textTable, textKey, oFind);
}

//This is pretty slow and hanging on main thread if the table is large
inline int GetRecordCount(DrMultiKeyTable* table) {
	if (!table) return 0;
	int count = 0;
	ForEachRecord(table, [&](DrEl*, size_t) { count++; return true; });
	return count;
}
//This is pretty slow and hanging on main thread if the table is large
inline int GetRecordCount(
	const Data::DataManager* dataManager,
	const wchar_t* tableName
) {
	auto table = GetTable(dataManager, tableName);
	return GetRecordCount(table);
}

using GetVersionInfoFunc = VersionInfo(*)(short);

inline bool IsVersionCompatible(PluginExecuteParams* params, GetVersionInfoFunc getVersionInfo) {
	static std::unordered_map<short, bool> versionCompatibleMap;
	auto it = versionCompatibleMap.find(params->table->_tabledef->type);
	if (it == versionCompatibleMap.end()) {
		VersionInfo compiledVersion = getVersionInfo(params->table->_tabledef->type);
		const auto& gameTableVersion = params->table->_tabledef->version;
		versionCompatibleMap[params->table->_tabledef->type] =
			(compiledVersion.key == gameTableVersion.ver);

		//Print message only the first time the version check fails
		if (compiledVersion.key != gameTableVersion.ver) {
			std::wstring msg = params->table->_tabledef->name;
			msg += L" table version mismatch:";
			msg += L"<br /> Compiled: " + std::to_wstring(compiledVersion.major) + L"." + std::to_wstring(compiledVersion.minor);
			msg += L"<br /> Game: " + std::to_wstring(gameTableVersion.major_ver) + L"." + std::to_wstring(gameTableVersion.minor_ver);
			msg += L"<br /> Plugin functionality for this table will be disabled.";
			params->displaySystemChatMessage(msg.c_str(), false);
		}

		it = versionCompatibleMap.find(params->table->_tabledef->type);
	}
	if (it != versionCompatibleMap.end()) {
		return it->second;
	}
	return false;
}

// Function pointer types
using PluginExecuteFunc = PluginReturnData(*)(PluginExecuteParams*);
using PluginIdentifierFunc = const char* (*)();
using PluginApiVersionFunc = int (*)();
using PluginVersionFunc = const char* (*)();
using PluginInitFunc = void (*)(PluginInitParams*);
using PluginUnregisterFunc = void (*)();
using PluginTableHandlerCountFunc = std::size_t(*)();
using PluginTableHandlersFunc = const PluginTableHandler* (*)();

// Macros to enforce plugin exports
#define DEFINE_PLUGIN_API_VERSION() \
    PLUGIN_EXPORT int PluginApiVersion() { return PLUGIN_API_VERSION; }

#define DEFINE_PLUGIN_IDENTIFIER(name) \
    PLUGIN_EXPORT const char* PluginIdentifier() { return name; }

#define DEFINE_PLUGIN_VERSION(ver) \
    PLUGIN_EXPORT const char* PluginVersion() { return ver; }

#define DEFINE_PLUGIN_INIT(initFn, unregisterFn) \
	PLUGIN_EXPORT void PluginInit(PluginInitParams* params) { initFn(params); } \
	PLUGIN_EXPORT void PluginUnregister() { unregisterFn(); }

#define DEFINE_PLUGIN_TABLE_HANDLERS(handlersArray) \
	PLUGIN_EXPORT std::size_t PluginTableHandlerCount() { return std::size(handlersArray); } \
	PLUGIN_EXPORT const PluginTableHandler* PluginTableHandlers() { return handlersArray; }

#define PLUGIN_DETOUR_GUARD(params, getTableVersionFunc) \
    auto __get_version_info = [](short type) -> VersionInfo { \
        auto v = getTableVersionFunc(type); \
        return VersionInfo{v.Version.VersionKey, v.Version.MajorVersion, v.Version.MinorVersion}; \
    }; \
    if (!(params) || !(params)->table || !(params)->dataManager || \
        !IsVersionCompatible(params, __get_version_info)) { \
        return {}; \
    }