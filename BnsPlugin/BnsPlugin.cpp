#include <pe/module.h>
#include "detours.h"
#include "BnsPlugin.h"
#include "searchers.h"
#include "Hooks.h"
#include "xorstr.hpp"
#include "BSFunctions.h"
#include "DatafileService.h"
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include "DatafilePluginManager.h"
#include "imgui_manager.h"
#include <functional>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Global state
static gsl::span<uint8_t> data;
static pe::module* module = nullptr;
static uintptr_t handle = 0;

static void ScannerSetup() {
#ifdef _DEBUG
	std::cout << "ScannerSetup" << std::endl;
#endif
	module = pe::get_module();
	handle = module->handle();
	const auto sections = module->segments();
	const auto it = std::ranges::find_if(sections, [](const IMAGE_SECTION_HEADER& x) {
		return x.Characteristics & IMAGE_SCN_CNT_CODE;
		});
	if (it != sections.end()) {
		data = it->as_bytes();
	}
}

_AddInstantNotification oAddInstantNotification = nullptr;

static void InitMessaging() {
#ifdef _DEBUG
	std::cout << "InitMessaging" << std::endl;
	std::cout << "Searching AddInstantNotification" << std::endl;
#endif
	bool diffPattern = false;
	auto sAddNotif = std::search(data.begin(), data.end(), pattern_searcher(xorstr_("45 33 DB 41 8D 42 ?? 3C 02 BB 05 00 00 00 41 0F 47 DB")));
	if (sAddNotif == data.end()) {
		diffPattern = true;
		sAddNotif = std::search(data.begin(), data.end(), pattern_searcher(xorstr_("33 FF 80 BC 24 80 00 00 00 01 75 05")));
	}
	if (sAddNotif != data.end()) {
		oAddInstantNotification = module->rva_to<std::remove_pointer_t<decltype(oAddInstantNotification)>>((uintptr_t)&sAddNotif[0] - (diffPattern ? 0x13 : 0x68) - handle);
	}
#ifdef _DEBUG
	std::cout << "Searching Done" << std::endl;
	printf("Address of AddInstantNotification is %p\n", (void*)oAddInstantNotification);
	std::cout << std::endl;
#endif
}

template<typename FuncType>
uintptr_t GetFunctionPtr(const char* pattern, int offset, FuncType& originalFunction, const char* debugName) {
	auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern));
	if (it != data.end()) {
		uintptr_t address = (uintptr_t)&it[0] + offset;
#ifdef _DEBUG
		printf("Address of %s is %p\n", debugName, (void*)address);
		std::cout << std::endl;
#endif
		originalFunction = module->rva_to<std::remove_pointer_t<FuncType>>(address - handle);
		return address;
	}
#ifdef _DEBUG
	printf("Failed to find %s\n", debugName);
	std::cout << std::endl;
#endif
	return 0;
}

template<typename FuncType>
uintptr_t HookFunction(const char* pattern, int offset, FuncType& originalFunction, FuncType hookFunction, const char* debugName) {
	auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern));
	if (it != data.end()) {
		uintptr_t address = (uintptr_t)&it[0] + offset;
#ifdef _DEBUG
		printf("Address of %s is %p\n", debugName, (void*)address);
		std::cout << std::endl;
#endif
		originalFunction = module->rva_to<std::remove_pointer_t<FuncType>>(address - handle);
		DetourAttach(reinterpret_cast<PVOID*>(&originalFunction), hookFunction);
		return address;
	}
#ifdef _DEBUG
	printf("Failed to find %s\n", debugName);
	std::cout << std::endl;
#endif
	return 0;
}

static __int64* HookDataManager(const char* pattern, int offset2) {
	auto it = std::search(data.begin(), data.end(), pattern_searcher(pattern));
	if (it != data.end()) {
		const auto aAddress = reinterpret_cast<uintptr_t>(&it[0]);
#ifdef _DEBUG
		printf("Address of %s is %p\n", "aAddress", (void*)aAddress);
		std::cout << std::endl;
#endif
		uintptr_t offset_addr = aAddress + offset2;
#ifdef _DEBUG
		printf("Address of %s is %p\n", "offset_addr", (void*)offset_addr);
		std::cout << std::endl;
#endif
		int offset = *reinterpret_cast<unsigned int*>(offset_addr);
#ifdef _DEBUG
		printf("offset is %d\n", offset);
		std::cout << std::endl;
#endif
		uintptr_t aData_DataManager_Effect = (offset_addr + offset + 4);
		uintptr_t dataManagerOffsetAddress = aData_DataManager_Effect + 3;
		int dataManagerOffset = *reinterpret_cast<unsigned int*>(dataManagerOffsetAddress);
		uintptr_t dataManagerAddress = dataManagerOffsetAddress + dataManagerOffset + 4;
		auto dataManagerPointer = reinterpret_cast<__int64*>(dataManagerAddress);
#ifdef _DEBUG
		printf("Address of %s is %p\n", "DataManagerEffect", (void*)aData_DataManager_Effect);
		std::cout << std::endl;
#endif
		return dataManagerPointer;
	}
	return nullptr;
}


static __int64* InitDetours() {
#ifdef _DEBUG
	std::cout << "InitDetours" << std::endl;
#endif

	DetourTransactionBegin();
	DetourUpdateThread(NtCurrentThread());

	auto pattern = xorstr_("0F B6 C0 85 C0 75 07 32 C0 E9 ?? 07 00 00 E8 ?? ?? ?? ?? 48 ?? ?? ?? ?? 00 00 00 48 ?? ?? ?? ?? 00 00 00 48 8B 00 48 8B ?? ?? ?? ?? 00 00 48 8B ?? ?? ?? ?? 00 00 FF 90 ?? 00 00 00 48 8B D0 48 ?? ?? ?? ??");
	auto dataManagerPtr = HookDataManager(pattern, 0xF);

	HookFunction(xorstr_("48 8B 49 14 E9 ?? ?? ?? ?? 48 81 C1 D4 01 00 00 48 8B 01 48 FF 60 18 CC"), -0x06, oFind_b8, hkFind_b8, "Find_b8");
	HookFunction(xorstr_("48 8B 49 14 E9 ?? ?? ?? ?? 48 3B 91 EC 01 00 00 73 0C 48 8B 81 D4 01 00 00 48 8B 04 D0 C3 33 C0 C3"), -0x06, oFind_b8AutoId, hkFind_b8AutoId, "Find_b8AutoId");

	GetFunctionPtr(xorstr_("48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 80 ?? ?? ?? ?? C3 C3 CC CC CC CC CC CC CC CC CC CC CC 48 8B 05"), 0x00, BNSClient_GetWorld, "BNSClient_GetWorld");

	DetourTransactionCommit();
	return dataManagerPtr;
}

Present_t oPresent = nullptr;
typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
ResizeBuffers_t oResizeBuffers = nullptr;

std::atomic<bool> g_runMainThreadInit{ false };
std::function<void()> g_mainThreadInitCallback;

static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	// Main-thread "tick"
	if (g_runMainThreadInit && g_mainThreadInitCallback) {
		g_mainThreadInitCallback();
		g_runMainThreadInit = false; // Only run once
	}
	ImGuiManager_OnPresent(pSwapChain, oPresent, pSwapChain, SyncInterval, Flags);
	return 0;
}

static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	// Release ImGui's render target view before resizing
	ImGuiManager_OnSwapchainResize();

	// Call the original ResizeBuffers
	return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static std::pair<void*, void*> GetPresentAndResizeAddr()
{
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 2;
	sd.BufferDesc.Height = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = GetForegroundWindow();
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	ID3D11Device* pDevice = nullptr;
	ID3D11DeviceContext* pContext = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, nullptr, &pContext);
	if (FAILED(hr)) return { nullptr, nullptr };

	void** vtable = *(void***)(pSwapChain);
	void* addr = vtable[8];
	void* addr2 = vtable[13];
	if (pSwapChain) pSwapChain->Release();
	if (pDevice) pDevice->Release();
	if (pContext) pContext->Release();
	return { addr, addr2 };
}

static DWORD WINAPI InitImgui() {
	auto [presentAddr, resizeBuffersAddr] = GetPresentAndResizeAddr();
	if (!presentAddr)
		return 1;

	oPresent = (Present_t)presentAddr;
	oResizeBuffers = (ResizeBuffers_t)resizeBuffersAddr;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)oPresent, hkPresent);
	DetourAttach(&(PVOID&)oResizeBuffers, hkResizeBuffers);
	DetourTransactionCommit();
	return 0;
}

static void InitDatafileService() {
	constexpr auto sleep_duration = std::chrono::milliseconds(1000);
	while (true) {
		if (g_DatafileService->CheckIfDatamanagerReady()) {
			// Notify main thread
			if (g_mainThreadInitCallback) {
				g_runMainThreadInit = true;
			}
			break;
		}
		std::this_thread::sleep_for(sleep_duration);
	}
}

static void BnsPlugin_Init() {
	ScannerSetup();
	InitMessaging();
	const auto dataManagerPtr = InitDetours();
	g_DatafileService = std::make_unique<DatafileService>(dataManagerPtr);
	if (dataManagerPtr != nullptr) {
		g_mainThreadInitCallback = []() {
			g_DatafilePluginManager = std::make_unique<DatafilePluginManager>("datafilePlugins");
			};
		std::jthread datafileServiceInitThread(InitDatafileService);
		datafileServiceInitThread.detach();
	}
	InitImgui();
}

void WINAPI BnsPlugin_Main() {
#ifdef _DEBUG
	AllocConsole();
	(void)freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "InitNotification: BNSR.exe" << std::endl;
#endif
	static std::once_flag once;
	std::call_once(once, BnsPlugin_Init);
}