#pragma once
#include "DrEl.h"
#include <map>
#include <iostream>
#include <set>
#include <vector>

struct DataChunk
{
	unsigned __int64 startId;
	unsigned __int64 endId;
	unsigned __int16 compressedSize;
	unsigned __int8* compressedData;
	unsigned __int16 rawDataSize;
	unsigned __int8* rawData;
	unsigned int elCount;
	unsigned __int16* elArray;
	DrEl* elPtrArray;
	unsigned int refCount;
	DataChunk* nextChunk;
	DataChunk* prevChunk;
};
struct DrTableDef;
struct DrCacheData
{
	struct __declspec(align(8)) ChunkList
	{
		DataChunk* MRU;
		DataChunk* LRU;
		unsigned int cacheSize;
	};
	unsigned int _chunkCount;
	unsigned int _maxCacheSize;
	unsigned int _chunkSize;
	unsigned __int16 _idOffset;
	DataChunk* _dataChunkArray;
	DrCacheData::ChunkList _dataChunkList;
	const DrTableDef* _tableDef;
	void* _tableCacheInfo;
	char padding[0x20];
};

#pragma pack(push, 1)

struct DrElIter;
struct DrInnerIter;
struct DrInnerIter_vtbl
{
	DrEl* (__fastcall* Ptr)(DrInnerIter* thisptr);
	bool(__fastcall* Next)(DrInnerIter* thisptr);
	bool(__fastcall* IsValid)(DrInnerIter* thisptr);
};
struct DrInnerIter
{
	DrInnerIter_vtbl* _vtptr;
};

struct DrAliasMap
{
	void* __vftable /*VFT*/;
};

struct DrLoaderDef;
struct DrDataTable;
struct /*VFT*/ DrDataTable_vtbl
{
	char padding_0_30[sizeof(void*) * 31];
	DrInnerIter* (__fastcall* createInnerIter)(DrDataTable* thisptr); //i = 31
	char padding_32_41[sizeof(void*) * 10];
	DrEl* (__fastcall* Find)(DrDataTable* thisptr, unsigned __int64); //i = 42
};

struct DrDataTable
{
	DrDataTable_vtbl* __vftable /*VFT*/;
};

struct DrTableDef;

struct __declspec(align(4)) DrDataTableImpl : DrDataTable {
	const DrTableDef* _tabledef;
	bool _uselowmemory;
	bool _useLegacyElHeader;
	bool _useTableCache;
	__declspec(align(2)) DrCacheData* _cacheData;
	void* _tableCacheInfo;
};

struct __declspec(align(4)) QuestFilterSet // sizeof=0x1C
{
	DrEl* _filterSetRecord;
	char* _filterRecords;
	void** _filters; //BnsTables::EU::quest_filter_Record**
	int _filterCount;
};

struct __declspec(align(4)) QuestReactionSet // sizeof=0x1C
{
	DrEl* _reactionSetRecord;
	char* _reactionRecords;
	char _reactionsPad[8];
	int _reactionCount;
};

struct __declspec(align(4)) QuestCase // sizeof=0x2C
{
	__int32 _caseCategory;
	DrEl* _caseRecord;
	QuestFilterSet* _filterSetArray;
	int _filterSetNum;
	QuestReactionSet* _reactionSetArray;
	int _reactionSetNum;
	char _questDecisionPad[8];
};

struct QuestReward // sizeof=0x20
{
	DrEl* _basicRecord;
	DrEl** _fixedRewardRecordArray;
	int _fixedRewardNum;
	DrEl** _optionalRewardRecordArray;
	int _optionalRewardNum;
};

struct Acquisition // sizeof=0x28
{
	DrEl* _acquisitionRecord;
	QuestCase* _caseArray;
	int _caseNum;
	DrEl** _acquisitionLossRecordArray;
	int _acquisitionLossRecordNum;
	QuestReward* _acquisitionReward;
};

struct quest_Record_KeyStub : DrEl {
public:
	union Key
	{
		struct {
			__int16 id;

		};
		unsigned __int64 key;
	};
	__declspec(align(8)) Key key;
};

struct __declspec(align(4)) Quest {
	quest_Record_KeyStub* _questRecord;
	Acquisition* _acquisition;
	DrEl* _missionStepRecordArray[16];
	void* _missionStepReactionSet[16];
	int _missionStepReactionSetNum[16];
	void* _missionArray[16];
	void* _missionStepFailArray[16];
	char _missionStepCompletedQuestDecisionPad[80];
	void* _completion;
	void* _transitArray[9];
	DrEl** _giveupLossRecordArray;
	int _giveupLossRecordNum;
};

struct __declspec(align(4))  QuestTableImpl : DrDataTable {
	unsigned __int16 _maxQuestId;
	unsigned __int16 _questListSize;
	Quest** _questArray;
	Quest** _questListArray;
	DrEl _el;
};

struct DrMultiKeyElMap;
struct DrMultiKeyElMap_vtbl {
	char padding[0x08];
	void(__fastcall* clearLink)(DrMultiKeyElMap* thisptr);
	void(__fastcall* construct)(DrMultiKeyElMap* thisptr, unsigned __int64);
	DrEl* (__fastcall* set)(DrMultiKeyElMap* thisptr, unsigned __int64, DrEl*, const bool);
	DrEl* (__fastcall* get)(DrMultiKeyElMap* thisptr, unsigned __int64);
	int(__fastcall* count)(DrMultiKeyElMap* thisptr);
};

struct DrMultiKeyElMap {
	DrMultiKeyElMap_vtbl* __vftable /*VFT*/;
	char padding[0x10];
	bool reconstructed;
	__unaligned __declspec(align(4)) void* elArray;
	int elCount;
};

//Just use MultiKeyTable for everything. The structures for AutokeyTables and DataCache are the same.
struct __declspec(align(4)) DrMultiKeyTable : DrDataTableImpl {
	DrMultiKeyElMap _elMap;
};

struct DrElIter
{
	DrInnerIter* _node;
	DrDataTable* _table;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct __declspec(align(4)) DrTableDef {
	const wchar_t* name;
	__int16 type;
	union Version {
		struct {
			__int16 major_ver;
			__int16 minor_ver;
		};
		unsigned __int32 ver;
	};
	Version version;
	__declspec(align(8)) void* elDefs;
	__int32 elCount;
	unsigned __int64 maxId;
	bool isAutoKey;
	__declspec(align(4)) __int32 module;
};
#pragma pack(pop)

#pragma pack(push, 4)
struct DrLoaderDef {
	DrDataTable* table;
	const DrTableDef* tableDef;

	const wchar_t* path;
	const wchar_t* xml;
	bool subdir;
	__declspec(align(4)) const wchar_t* xsd;
	bool(__fastcall* postproc)(DrEl*);
	char xmldoc[0x0C];
	char l10ndoc[0x0C];
	bool loaded;
	__declspec(align(4)) float tableCacheSize;
};
#pragma pack(pop)
#pragma pack(push, 1)
namespace Data {
	struct DataManager
	{
		char padding[0x48];
		void* _xmlReader;
		void* _readerIO;
		void* _readerLocalIO;
		void* _aliasMap;
		void* _elreader;
		char padding2[0x54];
#ifdef _BNSKR
		__declspec(align(4)) DrLoaderDef _loaderDefs[494];
#elif _BNSEU
		__declspec(align(4)) DrLoaderDef _loaderDefs[494];
#else
		__declspec(align(4)) DrLoaderDef _loaderDefs[494];
#endif
	};
}
#pragma pack(pop)

#pragma pack(push, 1)
struct PartyMemberProperty
{
	char pad0[8];
	__int64 hp;
	char level;
	char mastery_level;
	__int16 x;
	__int16 y;
	__int16 z;
	char pad1[2];
	int geo_zone;
	__int64 max_hp;
	int max_hp_equip;
	__int16 max_sp;
	__int16 yaw;
	char dead;
	char faction;
	char faction2;
	char pad2[1];
	int faction_score;
	char hp_alert;
	char pad3[7];
	__int64 newbie_info_flag;
	__int64 guard_gauge;
	__int64 max_guard_gauge;
	int max_guard_gauge_equip;
	char pad4[6];
};

struct FWindowsPlatformTime {
};

struct PreciseTimer // sizeof=0x10
{
	unsigned __int64 _startTime;
	FWindowsPlatformTime _timer;
	char pad0[3];
	float _limit;
};
struct Member {
	int _memberKey;
	char pad0[4];
	unsigned __int64 _creatureId;
	PartyMemberProperty _property;
	void* _manager;
	int _summonedDataId;
	int _zoneChannel;
	int _race;
	int _job;
	bool _inSight;
	bool _login;
	bool _banishAgreement;
	char pad1[5];
	/*std::vector<Member::MemberEffect> _memberEffectList;*/
	char padMemberEffect[0x18];
	PreciseTimer _logoutTimer;
	std::wstring _name;
	__int16 _worldId;
	bool _changedDeadState;
	char pad3[5];
	PreciseTimer _deadStateTimer;
	std::set<unsigned __int64> _aggroNormalNpcList;
	unsigned __int64 _aggroBoss1NpcId;
	unsigned __int64 _aggroBoss2NpcId;
	unsigned __int64 _aggroBoss3NpcId;
	unsigned __int64 _aggroBoss4NpcId;
	unsigned __int64 _aggroSummonedId;
	std::set<unsigned __int64> _summonedAggroNormalNpcList;
	unsigned __int64 _summonedAggroBoss1NpcId;
	unsigned __int64 _summonedAggroBoss2NpcId;
	bool _comebackSession;
	bool _reinforcement;
	char _newbieCareDungeonList[20];
	bool _newbieCareStae;
	char pad6[1];
};

struct Party {
	void* _vtbl;
	unsigned __int64 _partyId;
	//SimpleVector<Member*> _memberList;
	std::vector<Member*> _memberList;
};
struct PTPlayer;
struct Player /* : Creature */ { //the offsets need cleanup to confirm inheritance
	char pad0[0x08];
	unsigned __int64 id;
	char pad1[0x08];
	PTPlayer* _ptPlayer;
	char pad2[0xE60 - 16 - 16];
	Party* Party;
};

struct GameObject;
class World {
public:
	char unknown_0[0x50];
	bool _activated;
	bool _IsTerrainChanged;
	bool _isTransit;
	bool _isEnterWorld;
	bool _isEnterZone;
	bool _tryLeaveZone;
	char _leaveReason;
	char unknown_1[1];
	short _worldId;
	char unknown_2[6];
	__int64 _zoneId;
	int _geozoneId;
	int _prevGeozoneId;
	__int16 _arenaChatServerId;
	char pad3[2];
	int _clock;
	char _pcCafeCode;
	bool _isConnectedTestServer;
	char pad4[2];
	int _jackpotFaction1Score;
	int _jackpotFaction2Score;
	__int32 _keyboardModeConvertedResult;
	char pad5[56];
	Player* _player;
	void* _playerSummoned;
	/*char _creatureSquadPad[0x10];
	char _creatureChildSquadPad[0x10];*/
	std::map<__int64, std::vector<__int64>> _creatureSquad;
	std::map<__int64, __int64> _creatureChildSquad;
	void* _convoy;
	void* _closetGroup;
	void* _chatInput;
	void* _gameTip;
	void* _notificationCenter;
	void* _personalCustomize;
	std::map<unsigned __int64, GameObject*> _mgr;
};

#pragma pack(pop)

struct PresentationObject;

struct GameObject {
	enum TYPE : int
	{
		GO_NONE = 0,
		GO_PC = 1,
		GO_SUMMONED = 2,
		GO_PET = 3,
		GO_ZONE_NPC = 4,
		GO_ITEM = 8,
		GO_ZONE = 16,
		GO_ENV = 32,
		GO_TEAM = 64,
		GO_TEAM_PARTY = 65,
		GO_PARTY = 128,
		GO_FIELD_ITEM = 144,
		GO_GATHER_SOURCE = 160,
		GO_NPC_DEAD_BODY = 176,
		GO_CAMPFIRE = 192,
		GO_GUILD = 208,
		GO_DUEL_BOT = 240,
		GO_DUELBOT_SUMMONED = 241,
		GO_DROPPED_POUCH = 242
	};
	void* vtptr;
	unsigned __int64 id;
	bool _inSight;
	PresentationObject* _PTObject;
};

static GameObject::TYPE GetGameObjectType(unsigned __int64 id) {
	// Extract bits 48-63 (the upper 2 bytes of id)
	return static_cast<GameObject::TYPE>((id >> 48) & 0xFFFF);
}
static GameObject::TYPE GetGameObjectType(GameObject* gameObject) {
	return GetGameObjectType(gameObject->id);
}

struct EffectProperty {
	void* __vftable;
	__int16 pos;
	char pad[2];
	int data_id;
	int duration;
	char stack_count;
	char detach_count;
	char pad1[2];
	__int64 expiration_time;
};

struct Creature;
struct EffectCatalog {
	struct Item {
		EffectProperty prop;
		DrEl* effect; //BnsTables::EU::effect_Record* or BnsTables::KR::effect_Record* This is to avoid including BnsTables dependencies
		char pad[0x10];
		PreciseTimer durationTimer;
	};
	Creature* owner;
	std::vector<EffectCatalog::Item*> catalog;
	int effectCount;
	char pad[4];
	EffectProperty lastDetachedEffect;
};

struct PropString {
	short len;
	char pad[6];
	wchar_t* str;
};

struct Creature : GameObject {
	char pad10[0x80];
	PropString name;

	char pad9[12];
	signed char level;
	char pad8[3];
	int exp;
	signed char mastery_level;
	char pad7[3];
	__int64 mastery_exp;

	__int64 hp;

	__int64 guard_gauge;
	__int64 money;
	__int64 money_diff;

	char pad6[0x248 + 16];

	bool combat_mode;

	char pad5[0x9A7 - 16];

	// pos = 0xCD0 - C0
	EffectCatalog* effectCatalog[17];
	char radius[4];
	char pad4[4];
	__int64 _scoreHP;
	bool _outofSightByWarp;
	char pad3[7];
	char pad2[0x10];
	bool _notTargetableByEffect;
	char pad1[7];
	void* _playingActions;
	char _soulMaskId;
	char _soulMaskId2;
	bool _playedCombatDirecting;
	bool _directingFlagStun;
	bool _directingFlagDown;
	bool _directingFlagKneel;
	bool _directingFlagKnockBack;
	char pad[1];
	//end = 0xCD0
};

#pragma pack(push, 1)
struct Npc : Creature {
	//char padding[0xCD0];
	void* npcRecord; // BnsTables::EU::npc_Record* or BnsTables::KR::npc_Record* This is to avoid including BnsTables dependencies
	char pad2[0x40 - 0x08];
	__int64 _finalHp;
};

const struct PresentationObject {
	void* vfptr;
	GameObject* _gameObject;
};

const struct PTCreature : PresentationObject {
	char pad_0001[0x570 + 0x78];
};

struct PTControlledCreature : PTCreature {
	unsigned __int64 _target;
};

struct PTPlayer : PTControlledCreature {
	char pad5[0x3E50 + 0x88];
	unsigned __int64 _lastTargetId;
	char pad6[0xE0];
	unsigned __int64 _targetBossObjectId_slot_1;
	unsigned __int64 _targetBossObjectId_slot_2;
	unsigned __int64 _targetBossObjectId_slot_3;
	unsigned __int64 _targetBossObjectId_slot_4;
};

struct PresentationWorld {
	char pad[0x50];
	PTPlayer* _player;
};
#pragma pack(pop)

enum class MessageType {
	SystemChat,
	ScrollingHeadline,
	ScrollingHeadline2,
	ScrollingHeadlineBoss
};