#pragma once
#include <pe/module.h>
#include "detours.h"
#include "BnsPlugin.h"
#include "searchers.h"
#include "Hooks.h";
#include "xorstr.hpp"
#include "PluginConfig.h"
#include "BSFunctions.h"
#include "DatafileService.h"
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

gsl::span<uint8_t> data;
pe::module* module;
uintptr_t handle;

static void WINAPI ScannerSetup() {
#ifdef _DEBUG
	std::cout << "ScannerSetup" << std::endl;
#endif // _DEBUG
	module = pe::get_module();
	handle = module->handle();
	const auto sections2 = module->segments();
	const auto& s2 = std::ranges::find_if(sections2.begin(), sections2.end(), [](const IMAGE_SECTION_HEADER& x) {
		return x.Characteristics & IMAGE_SCN_CNT_CODE;
		});
	data = s2->as_bytes();
}

static void WINAPI InitConfigValues() {
	g_PluginConfig.ReloadFromConfig();
}

_AddInstantNotification oAddInstantNotification;

/// <summary>
/// Setup BnS messaging to send chat or notification messages in game.
/// From Tonic
/// </summary>
/// <returns></returns>
static void WINAPI InitMessaging() {
#ifdef _DEBUG
	std::cout << "InitMessaging" << std::endl;
#endif // _DEBUG

#ifdef _DEBUG
	std::cout << "Searching AddInstantNotification" << std::endl;
#endif // _DEBUG
	// Used for sending notifications about certain actions
	bool diffPattern = false;
	auto sAddNotif = std::search(data.begin(), data.end(), pattern_searcher(xorstr_("45 33 DB 41 8D 42 ?? 3C 02 BB 05 00 00 00 41 0F 47 DB")));
	if (sAddNotif == data.end()) {
		// Old compiler stuff (NAEU CLIENT)
		diffPattern = true;
		sAddNotif = std::search(data.begin(), data.end(), pattern_searcher(xorstr_("33 FF 80 BC 24 80 00 00 00 01 75 05")));
	}

	if (sAddNotif != data.end()) {
		oAddInstantNotification = module->rva_to<std::remove_pointer_t<decltype(oAddInstantNotification)>>((uintptr_t)&sAddNotif[0] - (diffPattern ? 0x13 : 0x68) - handle);
	}

#ifdef _DEBUG
	std::cout << "Searching Done" << std::endl;
#endif // _DEBUG

#ifdef _DEBUG
	printf("Address of AddInstantNotification is %p\n", (void*)oAddInstantNotification);
	std::cout << std::endl;
#endif // _DEBUG
}

/// <summary>
/// </summary>
/// <typeparam name="FuncType"></typeparam>
/// <param name="pattern"></param>
/// <param name="offset"></param>
/// <param name="originalFunction"></param>
/// <param name="debugName"></param>
/// <returns></returns>
template<typename FuncType>
uintptr_t GetFunctionPtr(const char* pattern, int offset, FuncType& originalFunction, const char* debugName)
{
	if (auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern)); it != data.end()) {
		uintptr_t address = (uintptr_t)&it[0] + offset;
#ifdef _DEBUG
		printf("Address of %s is %p\n", debugName, (void*)address);
		std::cout << std::endl;
#endif // _DEBUG
		originalFunction = module->rva_to<std::remove_pointer_t<FuncType>>(address - handle);
		return address;
	}
#ifdef _DEBUG
	printf("Failed to find %s\n", debugName);
	std::cout << std::endl;
#endif // _DEBUG
	return 0;
}

/// <summary>
/// </summary>
/// <typeparam name="FuncType"></typeparam>
/// <param name="pattern"></param>
/// <param name="offset"></param>
/// <param name="originalFunction"></param>
/// <param name="hookFunction"></param>
/// <param name="debugName"></param>
/// <returns></returns>
template<typename FuncType>
uintptr_t HookFunction(const char* pattern, int offset, FuncType& originalFunction, FuncType hookFunction, const char* debugName)
{
	if (auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern)); it != data.end()) {
		uintptr_t address = (uintptr_t)&it[0] + offset;
#ifdef _DEBUG
		printf("Address of %s is %p\n", debugName, (void*)address);
		std::cout << std::endl;
#endif // _DEBUG
		originalFunction = module->rva_to<std::remove_pointer_t<FuncType>>(address - handle);
		DetourAttach(&(PVOID&)originalFunction, hookFunction);
		return address;
	}
#ifdef _DEBUG
	printf("Failed to find %s\n", debugName);
	std::cout << std::endl;
#endif // _DEBUG
	return 0;
}

static __int64* HookDataManager(const char* pattern, int offset2) {
	if (auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern)); it != data.end()) {
		const auto aAddress = reinterpret_cast<uintptr_t>(&it[0]);
#ifdef _DEBUG
		printf("Address of %s is %p\n", "aAddress", (void*)aAddress);
		std::cout << std::endl;
#endif // _DEBUG
		uintptr_t offset_addr = aAddress + offset2;
#ifdef _DEBUG
		printf("Address of %s is %p\n", "offset_addr", (void*)offset_addr);
		std::cout << std::endl;
#endif // _DEBUG

		int offset = *reinterpret_cast<unsigned int*>(offset_addr);
#ifdef _DEBUG
		printf("offset is %d\n", offset);
		std::cout << std::endl;
#endif // _DEBUG
		uintptr_t aData_DataManager_Effect = (offset_addr + offset + 4);

		uintptr_t dataManagerOffsetAddress = aData_DataManager_Effect + 3;
		int dataManagerOffset = *reinterpret_cast<unsigned int*>(dataManagerOffsetAddress);
		uintptr_t dataManagerAddress = dataManagerOffsetAddress + dataManagerOffset + 4;
		auto dataManagerPointer = reinterpret_cast<__int64*>(dataManagerAddress);
#ifdef _DEBUG
		printf("Address of %s is %p\n", "DataManagerEffect", (void*)aData_DataManager_Effect);
		std::cout << std::endl;
#endif // _DEBUG
		return dataManagerPointer;
	}
	return nullptr;
}

static __int64* WINAPI InitDetours() {
#ifdef _DEBUG
	std::cout << "InitDetours" << std::endl;
#endif // _DEBUG
	DetourTransactionBegin();
	DetourUpdateThread(NtCurrentThread());

	HookFunction(xorstr_("48 85 D2 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 8B 03"), -0x15, oBUIWorld_ProcessEvent, &hkBUIWorld_ProcessEvent, "aBUIWorld_ProcessEvent");
	auto pattern = xorstr_("0F B6 C0 85 C0 75 07 32 C0 E9 ?? 07 00 00 E8 ?? ?? ?? ?? 48 ?? ?? ?? ?? 00 00 00 48 ?? ?? ?? ?? 00 00 00 48 8B 00 48 8B ?? ?? ?? ?? 00 00 48 8B ?? ?? ?? ?? 00 00 FF 90 ?? 00 00 00 48 8B D0 48 ?? ?? ?? ??");
	auto dataManagerPtr = HookDataManager(pattern, 0xF);

	//TODO: Check hooks
	//HookFunction(xorstr_("48 8B 49 14 E9 ?? ?? ?? ?? 48 81 C1 D4 01 00 00 48 8B 01 48 FF 60 18 CC"), -0x06, oFind_b8, hkFind_b8, "Find_b8"); //shorter function
	//HookFunction(xorstr_("48 8B 49 14 E9 ?? ?? ?? ?? 48 3B 91 EC 01 00 00 73 0C 48 8B 81 D4 01 00 00 48 8B 04 D0 C3 33 C0 C3"), -0x06, oFind_b8AutoId, hkFind_b8AutoId, "Find_b8AutoId"); //longer function

	GetFunctionPtr(xorstr_("48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 80 ?? ?? ?? ?? C3 C3 CC CC CC CC CC CC CC CC CC CC CC 48 8B 05"), 0x00, BNSClient_GetWorld, "BNSClient_GetWorld");

	DetourTransactionCommit();
	return dataManagerPtr;
}

static void InitDatafileService() {
	while (true) {
		if (g_DatafileService.Setup() || g_DatafileService.IsCriticalFail()) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

static void WINAPI BnsPlugin_Init()
{
	InitConfigValues();
	ScannerSetup();
	InitMessaging();
	const auto dataManagerPtr = InitDetours();
	g_DatafileService.SetDataManagerPtr(dataManagerPtr);
	if (dataManagerPtr != nullptr) {
		std::jthread datafileServiceInitThread(InitDatafileService);
		datafileServiceInitThread.detach();
	}
}

void WINAPI BnsPlugin_Main()
{
#ifdef _DEBUG
	//Open console and print (testing)
	AllocConsole();
	(void)freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);  //insecure
	std::cout << "InitNotification: BNSR.exe" << std::endl;
#endif
	static std::once_flag once;

	std::call_once(once, BnsPlugin_Init);
}