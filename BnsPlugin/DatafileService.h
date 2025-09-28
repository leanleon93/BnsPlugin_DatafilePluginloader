#pragma once
#include "Data.h"
#include <array>
#include <string>

enum UsedTable {
	SkillShow3,
};

class DatafileService {
public:
	bool Setup();
	bool IsSetupComplete() const;
	bool IsCriticalFail() const;
	Data::DataManager* GetDataManager();
	void SetDataManagerPtr(__int64 const* ptr);
	__int16 __fastcall GetTableId(const UsedTable usedTable) const;
private:
	__int64 const* dataManagerPtr;
	bool SetupComplete;
	bool CriticalFail;
	bool CompatabilityCheck();

	//ATTENTION: Has to be in the same order as the enum!
	const std::array<std::wstring, 1> usedTableNames = { L"skillshow3" }; // Adjust size if more enums are added 
	std::array<__int16, 1> tableIds = { -1 }; // Initialize with default values (-1 for "not found")
	std::array<bool, 1> versionCheckSuccess = { false }; // Initialize with default values (false)

	bool AllVersionsSuccess() const;
};

extern DatafileService g_DatafileService;