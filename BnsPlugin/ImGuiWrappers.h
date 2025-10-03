#pragma once
#include "imgui.h"
#include "imgui_plugin_api.h"

#pragma region ImguiWrappers
static void ImGui_TextColored_Wrapper(float r, float g, float b, float a, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	ImGui::TextColoredV(ImVec4(r, g, b, a), fmt, args);
	va_end(args);
}

static bool ImGui_ArrowButton_Wrapper(const char* str_id, int dir) {
	// Clamp dir to valid ImGuiDir values (0-3)
	if (dir < 0) dir = 0;
	if (dir > 3) dir = 3;
	return ImGui::ArrowButton(str_id, static_cast<ImGuiDir>(dir));
}

static bool ImGui_Button_Wrapper(const char* label) {
	return ImGui::Button(label);
}

// InputText: Remove flags/callbacks, just label, buf, and buf_size.
static bool ImGui_InputText_Wrapper(const char* label, char* buf, size_t buf_size) {
	// No extra flags/callbacks; just basic input
	return ImGui::InputText(label, buf, buf_size);
}

// InputInt: Remove extra params, just label and pointer to int.
static bool ImGui_InputInt_Wrapper(const char* label, int* v) {
	return ImGui::InputInt(label, v);
}

// InputFloat: Remove extra params, just label and pointer to float.
static bool ImGui_InputFloat_Wrapper(const char* label, float* v) {
	return ImGui::InputFloat(label, v);
}

// SliderInt: Only label, pointer, min, max
static bool ImGui_SliderInt_Wrapper(const char* label, int* v, int v_min, int v_max) {
	return ImGui::SliderInt(label, v, v_min, v_max);
}

// SliderFloat: Only label, pointer, min, max
static bool ImGui_SliderFloat_Wrapper(const char* label, float* v, float v_min, float v_max) {
	return ImGui::SliderFloat(label, v, v_min, v_max);
}

static bool ImGui_CollapsingHeader_Wrapper(const char* label) {
	return ImGui::CollapsingHeader(label);
}
static void ImGui_BeginChild_Wrapper(const char* str_id) {
	ImGui::BeginChild(str_id);
}

static void ImGui_OpenPopup_Wrapper(const char* str_id) {
	ImGui::OpenPopup(str_id);
}
static bool ImGui_BeginPopup_Wrapper(const char* str_id) {
	return ImGui::BeginPopup(str_id);
}
static bool ImGui_Begin_Wrapper(const char* name, bool* p_open) {
	return ImGui::Begin(name, p_open);
}
static void ImGui_Dummy_Wrapper(float w, float h) {
	ImGui::Dummy(ImVec2(w, h));
}
PluginImGuiAPI g_imguiApi = {
	&ImGui::Text,
	&ImGui_TextColored_Wrapper,
	&ImGui::Separator,
	&ImGui::SameLine,
	&ImGui_Button_Wrapper,
	&ImGui::SmallButton,
	&ImGui_ArrowButton_Wrapper,
	&ImGui::Checkbox,
	&ImGui::RadioButton,
	&ImGui::RadioButton,
	&ImGui_InputText_Wrapper,
	&ImGui_InputInt_Wrapper,
	&ImGui_InputFloat_Wrapper,
	&ImGui_SliderInt_Wrapper,
	&ImGui_SliderFloat_Wrapper,
	&ImGui::Combo,
	&ImGui_CollapsingHeader_Wrapper,
	&ImGui::TreeNode,
	&ImGui::TreePop,
	&ImGui_BeginChild_Wrapper,
	&ImGui::EndChild,
	&ImGui_OpenPopup_Wrapper,
	&ImGui_BeginPopup_Wrapper,
	&ImGui::EndPopup,
	&ImGui::BeginTooltip,
	&ImGui::EndTooltip,
	&ImGui::SetTooltip,
	&ImGui_Begin_Wrapper,
	&ImGui::End,
	&ImGui::Spacing,
	&ImGui_Dummy_Wrapper,
	&ImGui::Indent,
	&ImGui::Unindent,
	&ImGui::PushID,
	&ImGui::PushID,
	&ImGui::PopID
};
#pragma endregion