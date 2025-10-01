#include "imgui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <map>
#include <mutex>
#include <string>


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

void ImGuiManager_Render()
{
	if (!g_ImGuiPanelVisible)
		return;
	std::lock_guard<std::mutex> lock(g_PanelsMutex);
	ImGui::Begin("Plugin Panels");
	for (auto& [id, entry] : g_Panels) {
		if (ImGui::CollapsingHeader(entry.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			entry.fn(entry.userData); // Call the plugin's panel function
		}
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

	// Call original WndProc for all other cases
	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// Helper to get HWND from swapchain
static HWND GetSwapChainHWND(IDXGISwapChain* pSwapChain) {
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);
	return sd.OutputWindow;
}

static void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	if (g_mainRenderTargetView)
		return;
	ID3D11Texture2D* pBackBuffer = nullptr;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (pBackBuffer)
	{
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void ImGuiManager_OnPresent(IDXGISwapChain* pSwapChain, Present_t oPresent, IDXGISwapChain* swap, UINT sync, UINT flags)
{
	if (!g_Initialized)
	{
		if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice)))
			return;
		g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
		CreateRenderTarget(pSwapChain);
		g_hWnd = GetSwapChainHWND(pSwapChain);
		ImGuiManager_Init(g_hWnd, g_pd3dDevice, g_pd3dDeviceContext);
	}
	else
	{
		CreateRenderTarget(pSwapChain);
	}

	ImGuiManager_NewFrame();
	ImGuiManager_Render();

	oPresent(swap, sync, flags);
}