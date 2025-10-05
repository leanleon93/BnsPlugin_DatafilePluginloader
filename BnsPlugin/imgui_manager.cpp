#include "imgui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <map>
#include <mutex>
#include <string>
#include "DatafilePluginManager.h"
#include <windows.h>
#include <fstream>
#include <ctime>

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static bool g_Initialized = false;
static WNDPROC oWndProc = nullptr;

struct PanelEntry {
	std::string name;
	ImGuiPanelRenderFn fn;
	void* userData;
};
static std::map<int, PanelEntry> g_Panels;
static int g_NextPanelId = 1;
static std::mutex g_PanelsMutex;

extern "C" __declspec(dllexport)
int __stdcall RegisterImGuiPanel(const ImGuiPanelDesc* desc) {
	std::lock_guard<std::mutex> lock(g_PanelsMutex);
	int id = g_NextPanelId++;
	g_Panels[id] = { desc->name, desc->renderFn, desc->userData };
	return id;
}

extern "C" __declspec(dllexport)
void __stdcall UnregisterImGuiPanel(int handle) {
	std::lock_guard<std::mutex> lock(g_PanelsMutex);
	g_Panels.erase(handle);
}

void ImGuiManager_Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
	g_hWnd = hwnd;
	g_pd3dDevice = device;
	g_pd3dDeviceContext = context;

	ImGui::CreateContext();
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Hook WndProc for ImGui input
	oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)ImGuiManager_WndProc);

	g_Initialized = true;
}

void ImGuiManager_Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (oWndProc && g_hWnd) {
		SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
	}
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
	g_Initialized = false;
}

bool ImGuiManager_IsInitialized()
{
	return g_Initialized;
}

void ImGuiManager_NewFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}
static bool g_ImGuiPanelVisible = false;
bool do_reload = false;

#ifdef _DEBUG
static std::mutex g_LogMutex;
static const size_t kMaxLogSize = 1024 * 1024; // 1 MB

static void DebugLog(const char* msg)
{
	std::lock_guard<std::mutex> lock(g_LogMutex);

	// Check file size and clear if too large
	std::ifstream in("imgui_debug.log", std::ios::ate | std::ios::binary);
	if (in.is_open()) {
		std::streamsize size = in.tellg();
		in.close();
		if (size > static_cast<std::streamsize>(kMaxLogSize)) {
			std::ofstream clear("imgui_debug.log", std::ios::trunc);
			// Optionally, write a marker that the log was cleared
			if (clear.is_open()) {
				clear << "[LOG CLEARED DUE TO SIZE LIMIT]\n";
			}
		}
	}

	std::ofstream log("imgui_debug.log", std::ios::app);
	if (log.is_open()) {
		// Add timestamp
		std::time_t t = std::time(nullptr);
		char timebuf[32];
		std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
		log << "[" << timebuf << "] " << msg;
		// Ensure newline
		if (msg[0] && msg[strlen(msg) - 1] != '\n') log << "\n";
	}
}
#else
#define DebugLog(msg) ((void)0)
#endif

// C-style SEH wrapper: no std::string, only POD types
static void SafePanelCall_SEH(ImGuiPanelRenderFn fn, void* userData, const char* name) {
	__try {
		fn(userData);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef _DEBUG
		char buf[256];
		strncpy(buf, "ImGui panel SEH exception in: ", sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		strncat(buf, name, sizeof(buf) - strlen(buf) - 2);
		strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
		DebugLog(buf);
#endif
	}
}

// C++ wrapper (safe to use std::string here, but not in SEH function)
static void SafePanelCall(ImGuiPanelRenderFn fn, void* userData, const std::string& name) {
	SafePanelCall_SEH(fn, userData, name.c_str());
}

// Release RTV (call before swapchain resize or device loss)
static void ReleaseRenderTarget()
{
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
		DebugLog("ImGuiManager: Released main render target view.\n");
	}
}

// Always recreate RTV (robust for alt-tab, resize, device loss)
static void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	ReleaseRenderTarget();
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)) && pBackBuffer)
	{
		HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
		pBackBuffer->Release();
		if (FAILED(hr)) {
			DebugLog("ImGuiManager: Failed to create render target view!\n");
		}
		else {
			DebugLog("ImGuiManager: Created main render target view.\n");
		}
	}
	else
	{
		DebugLog("ImGuiManager: Failed to get backbuffer from swapchain!\n");
	}
}

void ImGuiManager_Render()
{
	if (!g_pd3dDevice || !g_pd3dDeviceContext || !g_mainRenderTargetView) {
		DebugLog("ImGuiManager_Render: D3D device/context/RTV is null!\n");
		return;
	}

	if (do_reload && g_DatafilePluginManager) {
		auto results = g_DatafilePluginManager->ReloadAll();
		do_reload = false;
	}

	if (!g_ImGuiPanelVisible)
		return;

	std::lock_guard<std::mutex> lock(g_PanelsMutex);
	ImGui::Begin("Datafile Plugins", &g_ImGuiPanelVisible);
	for (auto& [id, entry] : g_Panels) {
		ImGui::PushID(id);
		if (ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			SafePanelCall(entry.fn, entry.userData, entry.name);
		}
		ImGui::PopID();
	}
	ImGui::End();

	ImGui::Render();
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ImGuiManager_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Toggle panel with INSERT key (change VK_INSERT to your preferred key)
	if (msg == WM_KEYDOWN && wParam == VK_INSERT)
	{
		g_ImGuiPanelVisible = !g_ImGuiPanelVisible;
		DebugLog("ImGuiManager_WndProc: Toggled ImGui panel visibility.\n");
		return 0; // Eat the key
	}

	// If ImGui panel is visible, let ImGui handle input and block game input if it wants to capture
	if (g_ImGuiPanelVisible)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR))
			return 0; // Block game input
		if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))
			return 0; // Block game mouse input
	}

	// Defensive: check oWndProc
	if (!oWndProc) {
		DebugLog("ImGuiManager_WndProc: oWndProc is null!\n");
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	// Call original WndProc for all other cases
	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// Helper to get HWND from swapchain
static HWND GetSwapChainHWND(IDXGISwapChain* pSwapChain) {
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);
	return sd.OutputWindow;
}

// Call this before swapchain resize (if you hook ResizeBuffers)
void ImGuiManager_OnSwapchainResize()
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	ReleaseRenderTarget();
	DebugLog("ImGuiManager_OnSwapchainResize: Swapchain resize handled.\n");
}

void ImGuiManager_OnPresent(IDXGISwapChain* pSwapChain, Present_t oPresent, IDXGISwapChain* swap, UINT sync, UINT flags)
{
	if (!g_Initialized)
	{
		if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
			DebugLog("ImGuiManager_OnPresent: Failed to get D3D11 device from swapchain!\n");
			return;
		}
		g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
		DebugLog("ImGuiManager_OnPresent: D3D11 device/context created.\n");
		g_hWnd = GetSwapChainHWND(pSwapChain);
		ImGuiManager_Init(g_hWnd, g_pd3dDevice, g_pd3dDeviceContext);
	}

	// Only do ImGui work if the panel is visible
	if (g_ImGuiPanelVisible)
	{
		CreateRenderTarget(pSwapChain);
		ImGui_ImplDX11_CreateDeviceObjects();

		ImGuiManager_NewFrame();
		ImGuiManager_Render();
	}

	oPresent(swap, sync, flags);
}