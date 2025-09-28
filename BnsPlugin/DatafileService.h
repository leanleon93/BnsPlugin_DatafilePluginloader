#pragma once
#include "Data.h"
#include <array>
#include <string>

class DatafileService {
public:
	bool Setup();
	bool IsSetupComplete() const;
	bool IsCriticalFail() const;
	Data::DataManager* GetDataManager();
	void SetDataManagerPtr(__int64 const* ptr);
private:
	__int64 const* dataManagerPtr;
	bool SetupComplete;
	bool CriticalFail;
};

extern DatafileService g_DatafileService;