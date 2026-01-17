#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <set>
#include <vector>

enum class EngineKeyStateType {
	EKS_PRESSED = 0,
	EKS_RELEASED = 1,
	EKS_REPEAT = 2,
	EKS_DOUBLECLICK = 3,
	EKS_AXIS = 4
};

std::string EngineKeyStateString(EngineKeyStateType type);

struct EngineEvent;

struct EngineEventVtbl
{
	void* (__fastcall* __vecDelDtor)(EngineEvent* This, unsigned int);
	int(__fastcall* Id)(EngineEvent* This);
};

struct EngineEvent {
	EngineEventVtbl* vfptr;
	EngineEvent* _next;
	__int64 _etime;
};

struct EInputKeyEvent : EngineEvent {
	char _vKey;
	char padd_2[0x2];
	EngineKeyStateType KeyState;
	bool bCtrlPressed;
	bool bShiftPressed;
	bool bAltPressed;
};

typedef void(__cdecl* _AddInstantNotification)(
	void* thisptr,
	const wchar_t* text,
	const wchar_t* particleRef,
	const wchar_t* sound,
	char track,
	bool stopPreviousSound,
	bool headline2,
	bool boss_headline,
	bool chat,
	char category,
	const wchar_t* sound2);

typedef void* (__cdecl* _BNSClient_GetWorld)();

#ifndef BSMessaging_H
#define BSMessaging_H

class BSMessaging {
private:
	static void SendGameMessage_s(
		void* GameWorld,
		_AddInstantNotification* oAddInstantNotification,
		const wchar_t* text,
		const wchar_t* particleRef,
		const wchar_t* sound,
		char track,
		bool stopPreviousSound,
		bool headline2,
		bool boss_headline,
		bool chat,
		char category,
		const wchar_t* sound2);
public:
	static void DisplaySystemChatMessage(void*, _AddInstantNotification*, const wchar_t*, bool playSound);
	static void DisplayScrollingTextHeadline(void*, _AddInstantNotification*, const wchar_t*, bool playSound);
	static void DisplayScrollingTextHeadline2(void*, _AddInstantNotification*, const wchar_t*, bool playSound);
	static void DisplayScrollingTextHeadlineBoss(void*, _AddInstantNotification*, const wchar_t*, bool playSound);
};
#endif // !BSMessaging_H