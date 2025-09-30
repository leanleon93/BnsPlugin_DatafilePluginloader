#include "DatafilePluginsdk.h"
#include <unordered_set>
#include <EU/zoneenv2/AAA_zoneenv2_RecordBase.h>
#include <EU/zoneenv2/zoneenv2_chest_Record.h>
#include <EU/BnsTableNames.h>

class ArtifactPlugin {
public:
    ArtifactPlugin() = default;
    void Init(PluginInitParams* /*params*/) {
        // No Init logic for now
    }

    PluginReturnData HandleZoneenv2(PluginExecuteParams* params) {
        PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
        DrEl* recordBase = params->oFind(params->table, params->key);
        if (!recordBase) return {};
        BnsTables::EU::zoneenv2_Record* record = reinterpret_cast<BnsTables::EU::zoneenv2_Record*>(recordBase);
        if (record->subtype != BnsTables::EU::zoneenv2_chest_Record::SubType()) return {};
        BnsTables::EU::zoneenv2_chest_Record* chestRecord = reinterpret_cast<BnsTables::EU::zoneenv2_chest_Record*>(record);
        if (chestRecord->expedition_type != 1) return {};
        chestRecord->show_quest_indicator = true;
        chestRecord->init_enable = true;

        const std::wstring alias = chestRecord->alias;
        if (gakEntries.find(alias) != gakEntries.end()) {
            setChestRecordProperties(chestRecord, gakIndicator, gakIcon, gakIconOver);
        }
        else if (jungEntries.find(alias) != jungEntries.end()) {
            setChestRecordProperties(chestRecord, jungIndicator, jungIcon, jungIconOver);
        }
        else if (juEntries.find(alias) != juEntries.end()) {
            setChestRecordProperties(chestRecord, juIndicator, juIcon, juIconOver);
        }
        else if (wonEntries.find(alias) != wonEntries.end()) {
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

private:
    static constexpr const wchar_t* jungIndicator = L"00008603.Envguide_5510_2";
    static constexpr const wchar_t* jungIcon = L"00009499.WeaponHolder";
    static constexpr const wchar_t* jungIconOver = L"00009499.WeaponHolder_over";
    static constexpr const wchar_t* wonIndicator = L"00008603.Envguide_2000";
    static constexpr const wchar_t* wonIcon = L"00009499.StoneLantern";
    static constexpr const wchar_t* wonIconOver = L"00009499.StoneLantern_over";
    static constexpr const wchar_t* gakIndicator = L"00008603.Envguide_4453";
    static constexpr const wchar_t* gakIcon = L"00009499.Dungeon_FrozenArk_Lever";
    static constexpr const wchar_t* gakIconOver = L"00009499.Dungeon_FrozenArk_Lever_over";
    static constexpr const wchar_t* juIndicator = L"00008603.Envguide_3010";
    static constexpr const wchar_t* juIcon = L"00009499.Dungeon_NaryuPole";
    static constexpr const wchar_t* juIconOver = L"00009499.Dungeon_NaryuPole_over";

    const std::unordered_set<std::wstring> jungEntries = {
        L"e_chest_jeryoungrim_collectD",
        L"e_chest_Daesamak_collectD",
        L"e_chest_Suwal_collectD",
        L"e_chest_BaekCheong_collectD"
    };
    const std::unordered_set<std::wstring> wonEntries = {
        L"e_chest_jeryoungrim_collectB",
        L"e_chest_Daesamak_collectB",
        L"e_chest_Suwal_collectB",
        L"e_chest_BaekCheong_collectB"
    };
    const std::unordered_set<std::wstring> gakEntries = {
        L"e_chest_jeryoungrim_collectC",
        L"e_chest_Daesamak_collectC",
        L"e_chest_Suwal_collectC",
        L"e_chest_BaekCheong_collectC"
    };
    const std::unordered_set<std::wstring> juEntries = {
        L"e_chest_jeryoungrim_collectA",
        L"e_chest_Daesamak_collectA",
        L"e_chest_Suwal_collectA",
        L"e_chest_BaekCheong_collectA"
    };

    void setChestRecordProperties(
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
};

// Static instance pointer
template<typename T> T*& StaticInstance() { static T* inst = nullptr; return inst; }

static void __fastcall Init(PluginInitParams* params) {
    auto& plugin = StaticInstance<ArtifactPlugin>();
    if (!plugin) plugin = new ArtifactPlugin();
    plugin->Init(params);
}

static PluginReturnData __fastcall Zoneenv2Detour(PluginExecuteParams* params) {
    auto& plugin = StaticInstance<ArtifactPlugin>();
    if (plugin) return plugin->HandleZoneenv2(params);
    return {};
}

static PluginTableHandler handlers[] = {
    { L"zoneenv2", &Zoneenv2Detour }
};

DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER("ArtifactHelper")
DEFINE_PLUGIN_VERSION("1.1.1")
DEFINE_PLUGIN_INIT(Init)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)