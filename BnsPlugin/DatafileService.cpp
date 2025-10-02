#include "DatafileService.h"
#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

std::unique_ptr<DatafileService> g_DatafileService;

Data::DataManager* DatafileService::GetDataManager() {
	if (this->dataManagerPtr == nullptr || *this->dataManagerPtr == NULL) {
		return nullptr;
	}
	return reinterpret_cast<Data::DataManager*>(*this->dataManagerPtr);
}

void DatafileService::SetDataManagerPtr(__int64 const* ptr) {
	this->dataManagerPtr = ptr;
}


/// <summary>
/// Setup checks if the item table is accessible. Just to make sure the game has initialized the data manager.
/// </summary>
/// <returns></returns>
bool DatafileService::CheckIfDatamanagerReady() {
	if (this->dataManagerPtr == nullptr || *this->dataManagerPtr == NULL) {
		return false;
	}
	const auto manager = reinterpret_cast<Data::DataManager*>(*this->dataManagerPtr);

	if (auto table = DataHelper::GetTable(manager, tableName.c_str()); table == nullptr) {
		return false;
	}
	if (auto tableDef = DataHelper::GetTableDef(manager, tableName.c_str()); tableDef == nullptr)
	{
		return false;
	}
	SetSetupComplete(true);
	return true;
}