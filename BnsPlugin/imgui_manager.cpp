#include "imgui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static bool g_Initialized = false;
static WNDPROC oWndProc = nullptr;

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

void ImGuiManager_Render()
{
	// Show the ImGui demo window
	ImGui::ShowDemoWindow();

	ImGui::Render();
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ImGuiManager_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
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