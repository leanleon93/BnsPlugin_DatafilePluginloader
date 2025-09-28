#include "DatafileService.h"
#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

DatafileService g_DatafileService;

Data::DataManager* DatafileService::GetDataManager() {
	if (this->dataManagerPtr == nullptr || *this->dataManagerPtr == NULL) {
		return nullptr;
	}
	return reinterpret_cast<Data::DataManager*>(*this->dataManagerPtr);
}

void DatafileService::SetDataManagerPtr(__int64 const* ptr) {
	this->dataManagerPtr = ptr;
}


bool DatafileService::IsSetupComplete() const {
	return SetupComplete;
}

bool DatafileService::IsCriticalFail() const {
	return CriticalFail;
}


bool DatafileService::Setup() {
	SetupComplete = true;
	return true;
}