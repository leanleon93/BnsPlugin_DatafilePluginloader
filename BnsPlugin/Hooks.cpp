#include "Hooks.h"
#include <unordered_map>
#include <utility>
#include <string>
#include <string_view>
#include "DatafileService.h"
#include "DatafilePluginManager.h"

extern _AddInstantNotification oAddInstantNotification;

// Helper: thread-safe toggle key state (if hooks are called from multiple threads)
template <typename Callable>
inline void handleKeyEvent(const EInputKeyEvent* InputKeyEvent, int vKeyTarget, const Callable& onPress) {
	static thread_local std::unordered_map<int, bool> toggleKeys;
	if (vKeyTarget == 0 || !InputKeyEvent) return;
	if (InputKeyEvent->_vKey == vKeyTarget) {
		bool& toggleKey = toggleKeys.try_emplace(vKeyTarget, false).first->second;
		if (!toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_PRESSED) {
			toggleKey = true;
			onPress();
		} else if (toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_RELEASED) {
			toggleKey = false;
		}
	}
}

template <typename Callable>
inline void handleKeyEventWithModifiers(
	const EInputKeyEvent* InputKeyEvent,
	int vKeyTarget,
	bool alt,
	bool shift,
	bool ctrl,
	const Callable& onPress
) {
	static thread_local std::unordered_map<int, bool> toggleKeys;
	if (vKeyTarget == 0 || !InputKeyEvent) return;
	if (InputKeyEvent->_vKey == vKeyTarget) {
		bool& toggleKey = toggleKeys.try_emplace(vKeyTarget, false).first->second;
		if (!toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_PRESSED) {
			if ((alt == InputKeyEvent->bAltPressed) &&
				(shift == InputKeyEvent->bShiftPressed) &&
				(ctrl == InputKeyEvent->bCtrlPressed)) {
				toggleKey = true;
				onPress();
			}
		} else if (toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_RELEASED) {
			toggleKey = false;
		}
	}
}

World* (__fastcall* BNSClient_GetWorld)();

// Converts std::string to std::wstring (UTF-8 safe)
inline std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return {};
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
	if (size_needed <= 0) return {};
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstrTo[0], size_needed);
	return wstrTo;
}

bool(__fastcall* oBUIWorld_ProcessEvent)(uintptr_t* This, EInputKeyEvent* InputKeyEvent);
bool __fastcall hkBUIWorld_ProcessEvent(uintptr_t* This, EInputKeyEvent* InputKeyEvent) {
	if (!InputKeyEvent) return false;
	if (InputKeyEvent->vfptr->Id(InputKeyEvent) == 2) {
		handleKeyEventWithModifiers(InputKeyEvent, 0x4F, true, true, false, []() {
			auto results = g_DatafilePluginManager.ReloadAll();
			constexpr auto message = LR"(Datafile Plugins Reloaded)";
			auto* gameWorld = BNSClient_GetWorld();
			BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, message, false);
			for (const auto& res : results) {
				std::wstring ws = L"- " + StringToWString(res);
				BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, ws.c_str(), false);
			}
		});
	}
	return oBUIWorld_ProcessEvent(This, InputKeyEvent);
}

static void DisplaySystemChatMessage(const wchar_t* message, bool playSound) {
	auto* gameWorld = BNSClient_GetWorld();
	BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, message, playSound);
}

// Use stack allocation for PluginExecuteParams to avoid heap allocation in hooks
DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key) {
	PluginExecuteParams params{ g_DatafileService.GetDataManager(), thisptr, key, oFind_b8, false, &DisplaySystemChatMessage };
	if (auto* pluginResult = g_DatafilePluginManager.ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8(thisptr, key);
}

DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey) {
	PluginExecuteParams params{ g_DatafileService.GetDataManager(), thisptr, autokey, oFind_b8AutoId, true, &DisplaySystemChatMessage };
	if (auto* pluginResult = g_DatafilePluginManager.ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8AutoId(thisptr, autokey);
}