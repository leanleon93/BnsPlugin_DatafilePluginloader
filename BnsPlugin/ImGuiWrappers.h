#pragma once
#include "imgui.h"
#include "imgui_plugin_api.h"

#pragma region ImguiWrappers
static void ImGuiWrapper_TextColored(float r, float g, float b, float a, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	ImGui::TextColoredV(ImVec4(r, g, b, a), fmt, args);
	va_end(args);
}

static bool ImGuiWrapper_ArrowButton(const char* str_id, int dir) {
	// Clamp dir to valid ImGuiDir values (0-3)
	if (dir < 0) dir = 0;
	if (dir > 3) dir = 3;
	return ImGui::ArrowButton(str_id, static_cast<ImGuiDir>(dir));
}

static bool ImGuiWrapper_Button(const char* label) {
	return ImGui::Button(label);
}

static bool ImGuiWrapper_CustomButton(const char* label, const float x = 0, const float y = 0) {
	return ImGui::Button(label, { x, y });
}

// InputText: Remove flags/callbacks, just label, buf, and buf_size.
static bool ImGuiWrapper_InputText(const char* label, char* buf, size_t buf_size) {
	// No extra flags/callbacks; just basic input
	return ImGui::InputText(label, buf, buf_size);
}

// InputInt: Remove extra params, just label and pointer to int.
static bool ImGuiWrapper_InputInt(const char* label, int* v) {
	return ImGui::InputInt(label, v);
}

// InputFloat: Remove extra params, just label and pointer to float.
static bool ImGuiWrapper_InputFloat(const char* label, float* v) {
	return ImGui::InputFloat(label, v);
}

// SliderInt: Only label, pointer, min, max
static bool ImGuiWrapper_SliderInt(const char* label, int* v, int v_min, int v_max) {
	return ImGui::SliderInt(label, v, v_min, v_max);
}

// SliderFloat: Only label, pointer, min, max
static bool ImGuiWrapper_SliderFloat(const char* label, float* v, float v_min, float v_max) {
	return ImGui::SliderFloat(label, v, v_min, v_max);
}

static bool ImGuiWrapper_CollapsingHeader(const char* label) {
	return ImGui::CollapsingHeader(label);
}
static void ImGuiWrapper_BeginChild(const char* str_id) {
	ImGui::BeginChild(str_id);
}

static void ImGuiWrapper_OpenPopup(const char* str_id) {
	ImGui::OpenPopup(str_id);
}
static bool ImGuiWrapper_BeginPopup(const char* str_id) {
	return ImGui::BeginPopup(str_id);
}
static bool ImGuiWrapper_Begin(const char* name, bool* p_open, int windowFlags) {
	return ImGui::Begin(name, p_open, windowFlags);
}
static void ImGuiWrapper_Dummy(float w, float h) {
	ImGui::Dummy(ImVec2(w, h));
}
static void ImGuiWrapper_SetNextWindowSize(float w, float h, int cond) {
	ImGui::SetNextWindowSize(ImVec2(w, h), cond);
}

static void ImGuiWrapper_SetNextWindowPos(float x, float y, int cond) {
	ImGui::SetNextWindowPos(ImVec2(x, y), cond);
}

static void ImGuiWrapper_SetCursorPos(float x, float y) {
	ImGui::SetCursorPos(ImVec2(x, y));
}
static ImFont* betterFont = nullptr;

static void ImGuiWrapper_DisplayTextInCenter(const char* text, float fontSize, unsigned int color, float xOffset = 0.0f, float yOffset = 0.0f, bool outline = true, std::string fontPath = "") {
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

static void ImGuiWrapper_DisplayProgressBarInCenter(
	float progress,
	const char* label,
	const char* countdown,
	float barWidth,
	float barHeight,
	float fontSize,
	unsigned int /*color, ignored, we use our own*/,
	float xOffset = 0.0f,
	float yOffset = 0.0f,
	bool outline = true,
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
	ImU32 bgColor = IM_COL32(36, 43, 49, 160);         // dark gray, more transparent
	ImU32 fillColor = IM_COL32(255, 231, 131, 255);    // pale yellow fill
	ImU32 borderBlack = IM_COL32(11, 12, 18, 255);        // black inner border
	ImU32 borderWhite = IM_COL32(255, 255, 255, 160);  // white outer border, slightly transparent
	ImU32 textColor = IM_COL32(255, 255, 255, 255);    // white text
	ImU32 outlineColor = IM_COL32(0, 0, 0, 255);       // black outline

	float rounding = 0.0f; // sharp corners

	// Draw bar background
	drawList->AddRectFilled(barPos, barEnd, bgColor, rounding);

	// Draw bar fill
	ImVec2 fillEnd(barPos.x + barWidth * progress, barEnd.y);
	if (progress > 0.0f) {
		drawList->AddRectFilled(barPos, fillEnd, fillColor, rounding);
	}

	// Draw bicolor border: black inner, white outer (fading)
	float borderThickness = 1.0f;
	float outerBorderThickness = 2.5f;
	// Black inner border
	drawList->AddRect(barPos, barEnd, borderBlack, rounding, 0, borderThickness);
	// White outer border (slightly outside, faded)
	ImVec2 outerExpand(outerBorderThickness, outerBorderThickness);
	ImVec2 outerBarPos(barPos.x - outerBorderThickness, barPos.y - outerBorderThickness);
	ImVec2 outerBarEnd(barEnd.x + outerBorderThickness, barEnd.y + outerBorderThickness);
	drawList->AddRect(outerBarPos, outerBarEnd, borderWhite, rounding, 0, 1.5f);
	// Draw label above bar, left-aligned, with outline
	if (label && *label) {
		ImVec2 labelSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
		ImVec2 labelPos(x, y - labelSize.y - 2.0f);
		// Outline (8 directions)
		if (outline)
			for (int dx = -1; dx <= 1; ++dx) {
				for (int dy = -1; dy <= 1; ++dy) {
					if (dx == 0 && dy == 0) continue;
					drawList->AddText(font, fontSize, ImVec2(labelPos.x + dx, labelPos.y + dy), outlineColor, label);
				}
			}
		drawList->AddText(font, fontSize, labelPos, textColor, label);
	}

	// Draw countdown text centered in bar, with outline
	if (countdown && *countdown) {
		ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, countdown);
		ImVec2 textPos(
			x + (barWidth - textSize.x) * 0.5f,
			y + (barHeight - textSize.y) * 0.5f
		);
		// Outline (8 directions)
		if (outline)
			for (int dx = -1; dx <= 1; ++dx) {
				for (int dy = -1; dy <= 1; ++dy) {
					if (dx == 0 && dy == 0) continue;
					drawList->AddText(font, fontSize, ImVec2(textPos.x + dx, textPos.y + dy), outlineColor, countdown);
				}
			}
		drawList->AddText(font, fontSize, textPos, textColor, countdown);
	}
}

static void ImGuiWrapper_SameLineDefault() {
	ImGui::SameLine();
}

PluginImGuiAPI g_imguiApi = {
	&ImGui::Text,
	&ImGuiWrapper_TextColored,
	&ImGui::Separator,
	&ImGui::SameLine,
	&ImGuiWrapper_SameLineDefault,
	&ImGuiWrapper_Button,
	&ImGuiWrapper_CustomButton,
	&ImGui::SmallButton,
	&ImGuiWrapper_ArrowButton,
	&ImGui::Checkbox,
	&ImGui::RadioButton,
	&ImGui::RadioButton,
	&ImGuiWrapper_InputText,
	&ImGuiWrapper_InputInt,
	&ImGuiWrapper_InputFloat,
	&ImGuiWrapper_SliderInt,
	&ImGuiWrapper_SliderFloat,
	&ImGui::Combo,
	&ImGuiWrapper_CollapsingHeader,
	&ImGui::TreeNode,
	&ImGui::TreePop,
	&ImGuiWrapper_BeginChild,
	&ImGui::EndChild,
	&ImGuiWrapper_OpenPopup,
	&ImGuiWrapper_BeginPopup,
	&ImGui::EndPopup,
	&ImGui::BeginTooltip,
	&ImGui::EndTooltip,
	&ImGui::SetTooltip,
	&ImGui::IsItemHovered,
	&ImGuiWrapper_Begin,
	&ImGui::End,
	&ImGui::Spacing,
	&ImGuiWrapper_Dummy,
	&ImGui::Indent,
	&ImGui::Unindent,
	&ImGui::PushID,
	&ImGui::PushID,
	&ImGui::PopID,
	&ImGuiWrapper_SetNextWindowSize,
	&ImGuiWrapper_SetNextWindowPos,
	&ImGui::SetWindowFontScale,
	&ImGuiWrapper_SetCursorPos,
	&ImGuiWrapper_DisplayTextInCenter,
	&ImGuiWrapper_DisplayProgressBarInCenter,
	&ImGui::Columns,
	&ImGui::NextColumn
};
#pragma endregion