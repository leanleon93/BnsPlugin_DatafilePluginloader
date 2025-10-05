#include "Hooks.h"
#include <unordered_map>
#include <utility>
#include <string>
#include <string_view>
#include "DatafileService.h"
#include "DatafilePluginManager.h"

extern _AddInstantNotification oAddInstantNotification;

World* (__fastcall* BNSClient_GetWorld)();

void DisplayGameMessage(const wchar_t* message, bool playSound, MessageType type) {
	auto* gameWorld = BNSClient_GetWorld();
	switch (type) {
	case MessageType::SystemChat:
		BSMessaging::DisplaySystemChatMessage(gameWorld, &oAddInstantNotification, message, playSound);
		break;
	case MessageType::ScrollingHeadline:
		BSMessaging::DisplayScrollingTextHeadline(gameWorld, &oAddInstantNotification, message, playSound);
		break;
	case MessageType::ScrollingHeadline2:
		BSMessaging::DisplayScrollingTextHeadline2(gameWorld, &oAddInstantNotification, message, playSound);
		break;
	case MessageType::ScrollingHeadlineBoss:
		BSMessaging::DisplayScrollingTextHeadlineBoss(gameWorld, &oAddInstantNotification, message, playSound);
		break;
	}
}

// Use stack allocation for PluginExecuteParams to avoid heap allocation in hooks
DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key) {
	if (!g_DatafilePluginManager) {
		return oFind_b8(thisptr, key);
	}
	PluginExecuteParams params{ g_DatafileService->GetDataManager(), oFind_b8, BNSClient_GetWorld, &DisplayGameMessage, thisptr, key };
	if (auto* pluginResult = g_DatafilePluginManager->ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8(thisptr, key);
}

DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey) {
	if (!g_DatafilePluginManager) {
		return oFind_b8AutoId(thisptr, autokey);
	}
	PluginExecuteParams params{ g_DatafileService->GetDataManager(), oFind_b8AutoId, BNSClient_GetWorld, &DisplayGameMessage, thisptr, autokey };
	if (auto* pluginResult = g_DatafilePluginManager->ExecuteAll(&params)) {
		return pluginResult;
	}
	return oFind_b8AutoId(thisptr, autokey);
}