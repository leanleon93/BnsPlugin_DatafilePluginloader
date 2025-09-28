#pragma once
#include "DrEl.h"
#include <map>
#include "EU/BnsTableNames.h"
#include "KR/BnsTableNames.h"

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
	char padding[0xB0];
	DrEl* (__fastcall* Find)(DrDataTable* thisptr, const wchar_t*, const DrAliasMap*);
	DrEl* (__fastcall* Find_b8)(DrDataTable* thisptr, unsigned __int64);
	void(__fastcall* SetUseLowMemory)(DrDataTable* thisptr, bool);
	DrInnerIter* (__fastcall* createInnerIter)(DrDataTable* thisptr, DrEl* const);
	DrInnerIter* (__fastcall* createInnerIter_d0)(DrDataTable* thisptr);
	void(__fastcall* removeInnerIter)(DrDataTable* thisptr, DrInnerIter*);
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
		__declspec(align(4)) DrLoaderDef _loaderDefs[BnsTables::KR::TableCount];
#elif _BNSEU
		__declspec(align(4)) DrLoaderDef _loaderDefs[BnsTables::EU::TableCount];
#endif
	};
}
#pragma pack(pop)
class DataHelper {
public:
	static DrDataTable* GetTable(const Data::DataManager* dataManager, int tableId);
	static DrDataTable* GetTable(const Data::DataManager* dataManager, const wchar_t* tableName);
	static __int16 GetTableId(const Data::DataManager* dataManager, const wchar_t* tableName);
	static const DrTableDef* GetTableDef(const Data::DataManager* dataManager, const wchar_t* tableName);
	static DrEl* GetRecord(const Data::DataManager* dataManager, int tableId, __int64 key);
};
