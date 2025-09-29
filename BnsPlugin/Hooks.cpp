#include "Hooks.h"
#include <unordered_map>
#include "DatafileService.h"
#include "DatafilePluginManager.h"

extern _AddInstantNotification oAddInstantNotification;

template <typename Callable>
void handleKeyEvent(EInputKeyEvent const* InputKeyEvent, int vKeyTarget, const Callable& onPress) {
	static std::unordered_map<int, bool> toggleKeys;
	if (vKeyTarget == 0)  return;
	if (InputKeyEvent->_vKey == vKeyTarget) {
		bool& toggleKey = toggleKeys[vKeyTarget];
		if (!toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_PRESSED) {
			toggleKey = true;
			onPress();
		}
		else if (toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_RELEASED) {
			toggleKey = false;
		}
	}
}

template <typename Callable>
void handleKeyEventWithModifiers(
	EInputKeyEvent const* InputKeyEvent,
	int vKeyTarget,
	bool alt,
	bool shift,
	bool ctrl,
	const Callable& onPress
) {
	static std::unordered_map<int, bool> toggleKeys;
	if (vKeyTarget == 0)  return;
	if (InputKeyEvent->_vKey == vKeyTarget) {
		bool& toggleKey = toggleKeys[vKeyTarget];
		if (!toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_PRESSED) {
			// Check for Alt, Shift, and Ctrl modifiers
			if ((alt == InputKeyEvent->bAltPressed) &&
				(shift == InputKeyEvent->bShiftPressed) &&
				(ctrl == InputKeyEvent->bCtrlPressed)) {
				toggleKey = true;
				onPress();
			}
		}
		else if (toggleKey && InputKeyEvent->KeyState == EngineKeyStateType::EKS_RELEASED) {
			toggleKey = false;
		}
	}
}

World* (__fastcall* BNSClient_GetWorld)();

// Converts std::string to std::wstring
inline std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

bool(__fastcall* oBUIWorld_ProcessEvent)(uintptr_t* This, EInputKeyEvent* InputKeyEvent);
bool __fastcall hkBUIWorld_ProcessEvent(uintptr_t* This, EInputKeyEvent* InputKeyEvent) {
	if (!InputKeyEvent)
		return false;
	if (InputKeyEvent->vfptr->Id(InputKeyEvent) == 2) {
		handleKeyEventWithModifiers(InputKeyEvent, 0x4F, true, true, false, []() { //shift + alt + o
			auto results = g_DatafilePluginManager.ReloadAll();
			auto message = LR"(Datafile Plugins Reloaded)";
			auto gameWorld = BNSClient_GetWorld();
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
	auto gameWorld = BNSClient_GetWorld();
	BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, message, playSound);
}

/// <summary>
/// Hook into Datamanager resolving a table reference
/// </summary>
DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key) {
	auto* pluginResult = g_DatafilePluginManager.ExecuteAll(new PluginExecuteParams{ g_DatafileService.GetDataManager(), thisptr, key, oFind_b8, false, &DisplaySystemChatMessage });
	if (pluginResult != nullptr) {
		return pluginResult;
	}
	return oFind_b8(thisptr, key);
}

/// <summary>
/// Hook into Datamanager resolving a table reference with autokey
/// </summary>
DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey) {
	auto* pluginResult = g_DatafilePluginManager.ExecuteAll(new PluginExecuteParams{ g_DatafileService.GetDataManager(), thisptr, autokey, oFind_b8AutoId, true, &DisplaySystemChatMessage });
	if (pluginResult != nullptr) {
		return pluginResult;
	}
	return oFind_b8AutoId(thisptr, autokey);
}