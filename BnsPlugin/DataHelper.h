#pragma once
#include "Hooks.h"
#include "Data.h"
class DataHelper {
public:
	static DrDataTable* GetTable(const Data::DataManager* dataManager, int tableId);
	static DrDataTable* GetTable(const Data::DataManager* dataManager, const wchar_t* tableName);
	static __int16 GetTableId(const Data::DataManager* dataManager, const wchar_t* tableName);
	static const DrTableDef* GetTableDef(const Data::DataManager* dataManager, const wchar_t* tableName);
	static DrEl* GetRecord(const Data::DataManager* dataManager, int tableId, __int64 key);
	static DrEl* GetRecordAutoId(const Data::DataManager* dataManager, int tableId, __int64 autokey);
};