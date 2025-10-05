#pragma once
#include "BSFunctions.h"
#include "Data.h"

struct DrMultiKeyTable;
struct DrEl;

extern DrEl* (__fastcall* oFind_b8)(DrMultiKeyTable* thisptr, unsigned __int64 key);
DrEl* __fastcall hkFind_b8(DrMultiKeyTable* thisptr, unsigned __int64 key);

extern DrEl* (__fastcall* oFind_b8AutoId)(DrMultiKeyTable* thisptr, unsigned __int64 autokey);
DrEl* __fastcall hkFind_b8AutoId(DrMultiKeyTable* thisptr, unsigned __int64 autokey);

extern World* (__fastcall* BNSClient_GetWorld)();

void DisplayGameMessage(const wchar_t* message, bool playSound, MessageType type);