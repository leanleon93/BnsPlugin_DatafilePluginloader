#include "Hooks.h"
#include <unordered_map>
#include <utility>
#include <string>
#include <string_view>
#include "DatafileService.h"
#include "DatafilePluginManager.h"

extern _AddInstantNotification oAddInstantNotification;

World* (__fastcall* BNSClient_GetWorld)();


static void DisplaySystemChatMessage(const wchar_t* message, bool playSound) {
	auto* gameWorld = BNSClient_GetWorld();
	BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, message, playSound);
}

// Use stack allocation for PluginExecuteParams to avoid heap allocation in hooks
DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key) {
	PluginExecuteParams params{ g_DatafileService.GetDataManager(), thisptr, key, oFind_b8, false, &DisplaySystemChatMessage };
	if (auto* pluginResult = g_DatafilePluginManager->ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8(thisptr, key);
}

DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey) {
	PluginExecuteParams params{ g_DatafileService.GetDataManager(), thisptr, autokey, oFind_b8AutoId, true, &DisplaySystemChatMessage };
	if (auto* pluginResult = g_DatafilePluginManager->ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8AutoId(thisptr, autokey);
}