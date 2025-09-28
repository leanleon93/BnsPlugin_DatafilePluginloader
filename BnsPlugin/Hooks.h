#pragma once
#include "BSFunctions.h"
#include "Data.h"

extern bool(__fastcall* oBUIWorld_ProcessEvent)(uintptr_t* This, EInputKeyEvent* InputKeyEvent);
bool __fastcall hkBUIWorld_ProcessEvent(uintptr_t* This, EInputKeyEvent* InputKeyEvent);

extern DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key);

extern DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey);

extern World* (__fastcall* BNSClient_GetWorld)();