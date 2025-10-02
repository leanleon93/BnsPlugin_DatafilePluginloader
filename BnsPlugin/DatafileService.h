#pragma once
#include "Data.h"
#include <array>
#include <string>
#include <atomic>
#include <memory>

class DatafileService {
public:
	DatafileService(__int64 const* ptr) : dataManagerPtr(ptr) {}
	bool CheckIfDatamanagerReady();
	bool IsSetupComplete() const { return setupComplete.load(); }
	void SetSetupComplete(bool value) { setupComplete.store(value); }
	Data::DataManager* GetDataManager();
	void SetDataManagerPtr(__int64 const* ptr);
private:
	__int64 const* dataManagerPtr;
	const std::wstring tableName = L"item";
	std::atomic<bool> setupComplete{ false };
};

extern std::unique_ptr<DatafileService> g_DatafileService;