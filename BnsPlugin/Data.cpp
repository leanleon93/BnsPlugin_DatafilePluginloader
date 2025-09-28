#include "Data.h"
#include <iostream>

DrDataTable* DataHelper::GetTable(const Data::DataManager* dataManager, int tableId) {
	auto index = tableId - 1;
	if (dataManager == nullptr) {
		return nullptr;
	}
	auto loaderDef = dataManager->_loaderDefs[index];
	auto tableDef = loaderDef.tableDef;
	auto tableName = tableDef->name;
#ifdef _DEBUG
	std::wcout << L"Table name: " << tableName << std::endl;
#endif // _DEBUG
	return dataManager->_loaderDefs[index].table;
}

DrDataTable* DataHelper::GetTable(const Data::DataManager* dataManager, const wchar_t* tableName) {
	if (dataManager == nullptr) {
		return nullptr;
	}
	int arraySize = sizeof(dataManager->_loaderDefs) / sizeof(dataManager->_loaderDefs[0]);
	for (int i = 0; i < arraySize; i++) {
		auto loaderDef = dataManager->_loaderDefs[i];
		auto tableDef = loaderDef.tableDef;
		if (tableDef == nullptr) continue;
		auto name = tableDef->name;
		if (wcscmp(name, tableName) == 0) {
			return dataManager->_loaderDefs[i].table;
		}
	}
	return nullptr;
}

const DrTableDef* DataHelper::GetTableDef(const Data::DataManager* dataManager, const wchar_t* tableName) {
	if (dataManager == nullptr) {
		return nullptr;
	}
	int arraySize = sizeof(dataManager->_loaderDefs) / sizeof(dataManager->_loaderDefs[0]);
	for (int i = 0; i < arraySize; i++) {
		auto loaderDef = dataManager->_loaderDefs[i];
		auto tableDef = loaderDef.tableDef;
		if (tableDef == nullptr) continue;
		auto name = tableDef->name;
		if (wcscmp(name, tableName) == 0) {
			return tableDef;
		}
	}
	return nullptr;
}

DrEl* DataHelper::GetRecord(const Data::DataManager* dataManager, int tableId, __int64 key)
{
	if (dataManager == nullptr) {
		return nullptr;
	}
	auto table = GetTable(dataManager, tableId);
	if (table == nullptr) {
		return nullptr;
	}
	auto record = table->__vftable->Find_b8(table, key);
	return record;
}

__int16 DataHelper::GetTableId(const Data::DataManager* dataManager, const wchar_t* tableName) {
	if (dataManager == nullptr) {
		return 0;
	}
	int arraySize = sizeof(dataManager->_loaderDefs) / sizeof(dataManager->_loaderDefs[0]);
	for (int i = 0; i < arraySize; i++) {
		auto loaderDef = dataManager->_loaderDefs[i];
		auto tableDef = loaderDef.tableDef;
		if (tableDef == nullptr) continue;
		auto name = tableDef->name;
		if (wcscmp(name, tableName) == 0) {
			return tableDef->type;
		}
	}
	return 0;
}