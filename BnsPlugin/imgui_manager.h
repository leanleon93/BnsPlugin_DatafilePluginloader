#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>

// Call once after device/window is ready
void ImGuiManager_Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
void ImGuiManager_Shutdown();
void ImGuiManager_NewFrame();
void ImGuiManager_Render();

// Call this on present hook, returns true if ImGui was initialized
bool ImGuiManager_IsInitialized();

LRESULT CALLBACK ImGuiManager_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
// For use in your present hook
void ImGuiManager_OnPresent(IDXGISwapChain* pSwapChain, Present_t oPresent, IDXGISwapChain* swap, UINT sync, UINT flags);