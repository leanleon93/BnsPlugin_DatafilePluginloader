#pragma once

// Callback signature for UI panels
typedef void (*ImGuiPanelRenderFn)(void* userData);

struct ImGuiPanelDesc {
	const char* name;            // Name to display in the UI (collapsing header)
	ImGuiPanelRenderFn renderFn; // Function called every frame
	void* userData;              // Opaque pointer to plugin's state/data
};

// Function pointer types for registration
typedef int(__stdcall* RegisterImGuiPanelFn)(const ImGuiPanelDesc* desc);
typedef void(__stdcall* UnregisterImGuiPanelFn)(int handle);

struct PluginImGuiAPI {
	// Basic text and separators
	void (*Text)(const char* fmt, ...);
	void (*TextColored)(float r, float g, float b, float a, const char* fmt, ...);
	void (*Separator)();
	void (*SameLine)(float offset_from_start_x, float spacing);

	// Buttons and controls
	bool (*Button)(const char* label);
	bool (*SmallButton)(const char* label);
	bool (*ArrowButton)(const char* str_id, int dir); // dir: 0=left, 1=right, 2=up, 3=down

	// Widgets
	bool (*Checkbox)(const char* label, bool* v);
	bool (*RadioButton)(const char* label, bool active);
	bool (*RadioButtonInt)(const char* label, int* v, int v_button);
	bool (*InputText)(const char* label, char* buf, size_t buf_size);
	bool (*InputInt)(const char* label, int* v);
	bool (*InputFloat)(const char* label, float* v);

	bool (*SliderInt)(const char* label, int* v, int v_min, int v_max);
	bool (*SliderFloat)(const char* label, float* v, float v_min, float v_max);

	bool (*Combo)(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items);

	// Layout
	bool (*CollapsingHeader)(const char* label);

	// Tree
	bool (*TreeNode)(const char* label);
	void (*TreePop)();

	// List
	void (*BeginChild)(const char* str_id);
	void (*EndChild)();

	// Popups & Modals
	void (*OpenPopup)(const char* str_id);
	bool (*BeginPopup)(const char* str_id);
	void (*EndPopup)();

	// Tooltips
	bool (*BeginTooltip)();
	void (*EndTooltip)();
	void (*SetTooltip)(const char* fmt, ...);

	// Windows
	bool (*Begin)(const char* name, bool* p_open);
	void (*End)();

	// Misc
	void (*Spacing)();
	void (*Dummy)(float w, float h);
};