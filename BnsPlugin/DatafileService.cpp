#include "DatafileService.h"
#include <algorithm>
#include <Windows.h>
#include <EU/BnsTableNames.h>
#include <KR/BnsTableNames.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

using namespace BnsTables;

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

__int16 __fastcall DatafileService::GetTableId(const UsedTable usedTable) const {
	return tableIds[static_cast<size_t>(usedTable)];
}

bool DatafileService::IsSetupComplete() const {
	return SetupComplete;
}

bool DatafileService::IsCriticalFail() const {
	return CriticalFail;
}

bool DatafileService::AllVersionsSuccess() const {
	return std::ranges::all_of(versionCheckSuccess, [](bool success) { return success; });
}

bool DatafileService::CompatabilityCheck() {
	if (this->dataManagerPtr == nullptr || *this->dataManagerPtr == NULL) {
		return false;
	}
	const auto manager = reinterpret_cast<Data::DataManager*>(*this->dataManagerPtr);
	for (size_t i = 0; i < usedTableNames.size(); ++i) {
		const auto& tableName = usedTableNames[i];
		if (auto table = DataHelper::GetTable(manager, tableName.c_str()); table == nullptr) {
			return false;
		}
		auto tableDef = DataHelper::GetTableDef(manager, tableName.c_str());
		if (tableDef == nullptr) {
			continue;
		}
#if _BNSEU
		auto confirmedVersion = EU::TableNames::GetTableVersion(EU::TableNames::GetTableId(tableName));
#elif _BNSKR
		auto confirmedVersion = KR::TableNames::GetTableVersion(KR::TableNames::GetTableId(tableName));
#endif
#ifdef _DEBUG
		printf("Table %ls:\n", tableName.c_str());
		printf("\tID: %d\n", tableDef->type);
		printf("\tActual Version: %u (Major: %d, Minor: %d) | Confirmed Version: %u (Major: %d, Minor: %d)\n",
			tableDef->version.ver, tableDef->version.major_ver, tableDef->version.minor_ver,
			confirmedVersion.Version.VersionKey, confirmedVersion.Version.MajorVersion, confirmedVersion.Version.MinorVersion);
		if (tableDef->version.ver != confirmedVersion.Version.VersionKey) {
			printf("\tVersion Mismatch: Actual version does not match the confirmed version.\n");
		}
		printf("\tAddress of %s is %p\n", "type", &tableDef->type);
		printf("\n");
#endif // _DEBUG
		versionCheckSuccess[i] = tableDef->version.ver == confirmedVersion.Version.VersionKey;
		tableIds[i] = tableDef->type;
	}
	return true;
}


bool DatafileService::Setup() {
	if (auto successVersionCheck = CompatabilityCheck(); !successVersionCheck) return false;
	if (!AllVersionsSuccess()) {
		MessageBox(nullptr, L"Plugin version is not 100% compatible with the game version.\nSome Features might not work but your game will not break.\nPlease update the plugin if available.", L"Plugin Version Mismatch", MB_OK | MB_ICONWARNING);
	}
	SetupComplete = true;
	return true;
}