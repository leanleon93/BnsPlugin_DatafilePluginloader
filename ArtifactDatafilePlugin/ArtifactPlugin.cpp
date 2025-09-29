#include "DatafilePluginsdk.h"
#include <unordered_set>
#include <algorithm>
#include <array>
#include <EU/zoneenv2/AAA_zoneenv2_RecordBase.h>
#include <EU/zoneenv2/zoneenv2_chest_Record.h>
#include <EU/BnsTableNames.h>

static void __fastcall Init(PluginInitParams* /*params*/) {
	// No Init
}

namespace {
	constexpr const wchar_t* jungIndicator = L"00008603.Envguide_5510_2";
	constexpr const wchar_t* jungIcon = L"00009499.WeaponHolder";
	constexpr const wchar_t* jungIconOver = L"00009499.WeaponHolder_over";
	constexpr const wchar_t* wonIndicator = L"00008603.Envguide_2000";
	constexpr const wchar_t* wonIcon = L"00009499.StoneLantern";
	constexpr const wchar_t* wonIconOver = L"00009499.StoneLantern_over";
	constexpr const wchar_t* gakIndicator = L"00008603.Envguide_4453";
	constexpr const wchar_t* gakIcon = L"00009499.Dungeon_FrozenArk_Lever";
	constexpr const wchar_t* gakIconOver = L"00009499.Dungeon_FrozenArk_Lever_over";
	constexpr const wchar_t* juIndicator = L"00008603.Envguide_3010";
	constexpr const wchar_t* juIcon = L"00009499.Dungeon_NaryuPole";
	constexpr const wchar_t* juIconOver = L"00009499.Dungeon_NaryuPole_over";

	constexpr std::array jungEntries = {
		L"e_chest_jeryoungrim_collectD",
		L"e_chest_Daesamak_collectD",
		L"e_chest_Suwal_collectD",
		L"e_chest_BaekCheong_collectD"
	};
	constexpr std::array wonEntries = {
		L"e_chest_jeryoungrim_collectB",
		L"e_chest_Daesamak_collectB",
		L"e_chest_Suwal_collectB",
		L"e_chest_BaekCheong_collectB"
	};
	constexpr std::array gakEntries = {
		L"e_chest_jeryoungrim_collectC",
		L"e_chest_Daesamak_collectC",
		L"e_chest_Suwal_collectC",
		L"e_chest_BaekCheong_collectC"
	};
	constexpr std::array juEntries = {
		L"e_chest_jeryoungrim_collectA",
		L"e_chest_Daesamak_collectA",
		L"e_chest_Suwal_collectA",
		L"e_chest_BaekCheong_collectA"
	};

	constexpr auto contains = [](const auto& arr, const std::wstring& value) -> bool {
		return std::ranges::find(arr, value) != arr.end();
		};

	inline void setChestRecordProperties(
		BnsTables::EU::zoneenv2_chest_Record* chestRecord,
		const wchar_t* indicator,
		const wchar_t* icon,
		const wchar_t* iconOver
	) {
		chestRecord->default_indicator_image = const_cast<wchar_t*>(indicator);
		chestRecord->mapunit_image_enable_close_true_imageset = const_cast<wchar_t*>(icon);
		chestRecord->mapunit_image_enable_close_true_over_imageset = const_cast<wchar_t*>(iconOver);
		chestRecord->mapunit_image_enable_close_false_imageset = const_cast<wchar_t*>(icon);
		chestRecord->mapunit_image_enable_close_false_over_imageset = const_cast<wchar_t*>(iconOver);
		chestRecord->mapunit_image_enable_open_imageset = const_cast<wchar_t*>(icon);
		chestRecord->mapunit_image_enable_open_over_imageset = const_cast<wchar_t*>(iconOver);
	}
} // namespace

static bool versionIncompatible = false;

static PluginReturnData __fastcall Zoneenv2Detour(PluginExecuteParams* params) {
	if (versionIncompatible || !params || !params->table || !params->dataManager) {
		return {};
	}
	const auto compiledTableVersion = BnsTables::EU::TableNames::GetTableVersion(params->table->_tabledef->type);
	const auto& gameTableVersion = params->table->_tabledef->version;
	if (compiledTableVersion.Version.VersionKey != gameTableVersion.ver) {
		versionIncompatible = true;
		params->displaySystemChatMessage(L"ArtifactPlugin: zoneenv2 table version mismatch. Plugin disabled.", false);
		return {};
	}
	DrEl* recordBase = params->oFind(params->table, params->key);
	if (!recordBase) return {};
	BnsTables::EU::zoneenv2_Record* record = reinterpret_cast<BnsTables::EU::zoneenv2_Record*>(recordBase);
	if (record->subtype != BnsTables::EU::zoneenv2_chest_Record::SubType()) return {};
	BnsTables::EU::zoneenv2_chest_Record* chestRecord = reinterpret_cast<BnsTables::EU::zoneenv2_chest_Record*>(record);
	if (chestRecord->expedition_type != 1) return {};
	chestRecord->show_quest_indicator = true;
	chestRecord->init_enable = true;

	if (contains(gakEntries, chestRecord->alias)) {
		setChestRecordProperties(chestRecord, gakIndicator, gakIcon, gakIconOver);
	}
	else if (contains(jungEntries, chestRecord->alias)) {
		setChestRecordProperties(chestRecord, jungIndicator, jungIcon, jungIconOver);
	}
	else if (contains(juEntries, chestRecord->alias)) {
		setChestRecordProperties(chestRecord, juIndicator, juIcon, juIconOver);
	}
	else if (contains(wonEntries, chestRecord->alias)) {
		setChestRecordProperties(chestRecord, wonIndicator, wonIcon, wonIconOver);
	}

	chestRecord->mapunit_image_enable_close_true_size_x = 23;
	chestRecord->mapunit_image_enable_close_true_size_y = 23;
	chestRecord->mapunit_image_enable_close_false_size_x = 23;
	chestRecord->mapunit_image_enable_close_false_size_y = 23;
	chestRecord->mapunit_image_enable_open_size_x = 23;
	chestRecord->mapunit_image_enable_open_size_y = 23;
	chestRecord->mapunit_image_disable_size_x = 23;
	chestRecord->mapunit_image_disable_size_y = 23;
	chestRecord->mapunit_image_unconfirmed_size_x = 18;
	chestRecord->mapunit_image_unconfirmed_size_y = 18;

	return {};
}

static PluginTableHandler handlers[] = {
	{ L"zoneenv2", &Zoneenv2Detour }
};

DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ArtifactPlugin")
DEFINE_PLUGIN_VERSION("1.1.0")
DEFINE_PLUGIN_INIT(Init)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)