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
static bool ImGui_Begin_Wrapper(const char* name, bool* p_open, int windowFlags) {
	return ImGui::Begin(name, p_open, windowFlags);
}
static void ImGui_Dummy_Wrapper(float w, float h) {
	ImGui::Dummy(ImVec2(w, h));
}
static void ImGui_SetNextWindowSize_Wrapper(float w, float h, int cond) {
	ImGui::SetNextWindowSize(ImVec2(w, h), cond);
}

static void ImGui_SetNextWindowPos_Wrapper(float x, float y, int cond) {
	ImGui::SetNextWindowPos(ImVec2(x, y), cond);
}

static void ImGui_SetCursorPos_Wrapper(float x, float y) {
	ImGui::SetCursorPos(ImVec2(x, y));
}
static ImFont* betterFont = nullptr;

static void DisplayTextInCenter_Impl(const char* text, float fontSize, unsigned int color, float xOffset = 0.0f, float yOffset = 0.0f, bool outline = true, std::string fontPath = "") {
	if (!text) return;
	ImGuiIO& io = ImGui::GetIO();
	if (betterFont == nullptr && !fontPath.empty()) {
		betterFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
	}
	ImFont* font = betterFont ? betterFont : ImGui::GetFont();
	float defaultFontSize = ImGui::GetFontSize();
	if (!font) return;
	if (fontSize <= 0.0f) fontSize = defaultFontSize;

	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
	ImVec2 pos(
		(io.DisplaySize.x - textSize.x) * 0.5f + xOffset,
		(io.DisplaySize.y - textSize.y) * 0.5f + yOffset
	);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	ImU32 outlineColor = IM_COL32(0, 0, 0, 255); // Black outline
	const float outlineThickness = 1.5f;

	// Draw outline (8 directions)
	if (outline)
		for (int dx = -1; dx <= 1; ++dx) {
			for (int dy = -1; dy <= 1; ++dy) {
				if (dx == 0 && dy == 0) continue;
				drawList->AddText(font, fontSize, ImVec2(pos.x + dx * outlineThickness, pos.y + dy * outlineThickness), outlineColor, text);
			}
		}
	// Draw main text
	drawList->AddText(font, fontSize, pos, color, text);
}

static void DisplayProgressBarInCenter_Impl(
	float progress,
	const char* label,
	const char* countdown,
	float barWidth,
	float barHeight,
	float fontSize,
	unsigned int /*color, ignored, we use our own*/,
	float xOffset = 0.0f,
	float yOffset = 0.0f,
	std::string fontPath = ""
) {
	ImGuiIO& io = ImGui::GetIO();
	if (betterFont == nullptr && !fontPath.empty()) {
		betterFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
	}
	ImFont* font = betterFont ? betterFont : ImGui::GetFont();
	float defaultFontSize = ImGui::GetFontSize();
	if (!font) return;
	if (fontSize <= 0.0f) fontSize = defaultFontSize;

	// Position at the screen center, with offsets
	float x = (io.DisplaySize.x - barWidth) * 0.5f + xOffset;
	float y = (io.DisplaySize.y - barHeight) * 0.5f + yOffset;
	ImVec2 barPos(x, y);
	ImVec2 barEnd(x + barWidth, y + barHeight);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	// Colors
	ImU32 bgColor = IM_COL32(24, 28, 36, 220);      // dark background
	ImU32 fillColor = IM_COL32(80, 180, 255, 255);  // light blue fill
	ImU32 borderColor = IM_COL32(60, 80, 120, 255); // border
	ImU32 textColor = IM_COL32(240, 240, 255, 255); // bright text for inside bar
	ImU32 textShadow = IM_COL32(0, 0, 0, 120);      // subtle shadow

	float rounding = 0.0f; // No rounding for sharp corners

	// Draw bar background
	drawList->AddRectFilled(barPos, barEnd, bgColor, rounding);

	// Draw bar fill
	ImVec2 fillEnd(barPos.x + barWidth * progress, barEnd.y);
	if (progress > 0.0f) {
		drawList->AddRectFilled(barPos, fillEnd, fillColor, rounding);
	}

	// Draw border
	drawList->AddRect(barPos, barEnd, borderColor, rounding, 0, 2.0f);

	// Calculate text sizes
	ImVec2 labelSize(0, 0), countdownSize(0, 0);
	if (label && *label) labelSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
	if (countdown && *countdown) countdownSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, countdown);

	// Vertically center the text in the bar
	float textY = y + (barHeight - labelSize.y) * 0.5f;

	// Padding from left/right edges
	const float sidePadding = 12.0f;

	// Draw label (left side)
	if (label && *label) {
		ImVec2 labelPos(x + sidePadding, textY);
		drawList->AddText(font, fontSize, ImVec2(labelPos.x + 1, labelPos.y + 1), textShadow, label);
		drawList->AddText(font, fontSize, labelPos, textColor, label);
	}

	// Draw countdown (right side)
	if (countdown && *countdown) {
		ImVec2 cdPos(x + barWidth - countdownSize.x - sidePadding, textY);
		drawList->AddText(font, fontSize, ImVec2(cdPos.x + 1, cdPos.y + 1), textShadow, countdown);
		drawList->AddText(font, fontSize, cdPos, textColor, countdown);
	}
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
	&ImGui::IsItemHovered,
	&ImGui_Begin_Wrapper,
	&ImGui::End,
	&ImGui::Spacing,
	&ImGui_Dummy_Wrapper,
	&ImGui::Indent,
	&ImGui::Unindent,
	&ImGui::PushID,
	&ImGui::PushID,
	&ImGui::PopID,
	&ImGui_SetNextWindowSize_Wrapper,
	&ImGui_SetNextWindowPos_Wrapper,
	&ImGui::SetWindowFontScale,
	&ImGui_SetCursorPos_Wrapper,
	&DisplayTextInCenter_Impl,
	&DisplayProgressBarInCenter_Impl
};
#pragma endregion