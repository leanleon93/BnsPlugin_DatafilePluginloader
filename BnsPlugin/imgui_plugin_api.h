#pragma once
typedef void (*ImGuiPanelRenderFn)(void* userData);

struct ImGuiPanelDesc {
	const char* name;                // Panel name (shown as header/collapsible)
	ImGuiPanelRenderFn renderFn;     // Draw function (called every frame)
	void* userData;                  // State pointer, passed to renderFn
};

// Function pointer types for registration
typedef int  (*RegisterImGuiPanelFn)(const ImGuiPanelDesc* desc);
typedef void (*UnregisterImGuiPanelFn)(int handle);