#include "d/d_file_select.h"
#include "d/d_s_name.h"

#include <array>
#include <cstring>

namespace new_save_modes {

namespace {

DEFINE_HOOK(&dFile_select_c::nameInput2, IntroSkipNameInput2);
DEFINE_HOOK(&dScnName_c::changeGameScene, IntroSkipNameSceneChange);
DEFINE_HOOK(
    static_cast<void (*)(const char*, s16, s8, s8, f32, u32, int, s8, s16, int, int)>(
        &dComIfGp_setNextStage),
    IntroSkipSetNextStage);

struct SaveByte {
    u16 offset;
    u8 value;
};

constexpr SaveByte kIntroSkipSaveBytes[] = {
    {1, 0x10}, {3, 0x0C}, {5, 0x1F}, {6, 0x54}, {7, 0x60}, {8, 0x4A}, {9, 0x19},
    {11, 0x01}, {12, 0x17}, {13, 0xFF}, {14, 0xFF}, {15, 0xFF}, {16, 0xFF},
    {17, 0xFF}, {18, 0xFF}, {19, 0x2F}, {20, 0x28}, {21, 0x2A}, {22, 0xFF},
    {23, 0xFF}, {41, 0x77}, {42, 0xD0}, {43, 0x1A}, {44, 0x54}, {45, 0x3C},
    {46, 0xF0}, {47, 0xF5}, {48, 0x01}, {49, 0x01}, {52, 0x43}, {53, 0x34},
    {54, 0x18}, {55, 0x90}, {64, 0xC4}, {65, 0xFD}, {66, 0xCB}, {67, 0xAF},
    {68, 0x43}, {69, 0x72}, {70, 0x68}, {71, 0xF6}, {72, 0xC6}, {73, 0x17},
    {74, 0x1E}, {75, 0x47}, {78, 0x46}, {79, 0x5F}, {80, 0x53}, {81, 0x50},
    {82, 0x31}, {83, 0x30}, {84, 0x34}, {87, 0x01}, {88, 0x46}, {89, 0x5F},
    {90, 0x53}, {91, 0x50}, {92, 0x31}, {93, 0x30}, {94, 0x38}, {98, 0x15},
    {100, 0xC6}, {101, 0x74}, {102, 0x20}, {103, 0x0C}, {104, 0x42}, {105, 0x14},
    {108, 0xC5}, {109, 0xAC}, {110, 0x79}, {111, 0x1B}, {112, 0xA7}, {113, 0x1D},
    {114, 0x46}, {115, 0x5F}, {116, 0x53}, {117, 0x50}, {118, 0x31}, {119, 0x30},
    {120, 0x38}, {122, 0x02}, {123, 0x02}, {124, 0x01}, {125, 0x06}, {152, 0xFF},
    {156, 0xFF}, {157, 0x48}, {158, 0xFF}, {159, 0xFF}, {160, 0xFF}, {161, 0xFF},
    {162, 0xFF}, {163, 0xFF}, {164, 0xFF}, {165, 0xFF}, {166, 0xFF}, {167, 0x65},
    {168, 0xFF}, {169, 0xFF}, {170, 0xFF}, {171, 0xFF}, {172, 0xFF}, {173, 0xFF},
    {174, 0xFF}, {175, 0xFF}, {176, 0x4A}, {177, 0xFF}, {178, 0xFF}, {179, 0x4B},
    {180, 0x01}, {181, 0x17}, {182, 0x14}, {183, 0x0B}, {184, 0xFF}, {185, 0xFF},
    {186, 0xFF}, {187, 0xFF}, {188, 0xFF}, {189, 0xFF}, {190, 0xFF}, {191, 0xFF},
    {192, 0xFF}, {193, 0xFF}, {194, 0xFF}, {195, 0xFF}, {196, 0xFF}, {197, 0xFF},
    {198, 0xFF}, {199, 0xFF}, {200, 0xFF}, {201, 0xFF}, {202, 0xFF}, {203, 0xFF},
    {205, 0x04}, {207, 0x0F}, {208, 0x80}, {210, 0x85}, {211, 0x02}, {214, 0x0D},
    {219, 0x20}, {227, 0x02}, {234, 0x40}, {244, 0x32}, {248, 0x1E}, {249, 0x1E},
    {250, 0x0F}, {251, 0x1E}, {252, 0x1E}, {253, 0x1E}, {254, 0x0A}, {256, 0x01},
    {257, 0x05}, {258, 0x01}, {267, 0xFF}, {276, 0x10}, {280, 0x01}, {375, 0x01},
    {401, 0x0B}, {427, 0x1C}, {428, 0x6A}, {429, 0x7A}, {430, 0x68}, {431, 0xB5},
    {436, 0x4C}, {437, 0x69}, {438, 0x6E}, {439, 0x6B}, {453, 0x45}, {454, 0x70},
    {455, 0x6F}, {456, 0x6E}, {457, 0x61}, {480, 0x01}, {481, 0x01}, {483, 0x01},
    {486, 0x01}, {487, 0x5E}, {491, 0x01}, {499, 0x10}, {504, 0x7F}, {505, 0xEA},
    {506, 0x3F}, {507, 0xBA}, {508, 0x1F}, {509, 0x9B}, {510, 0xAE}, {511, 0x2F},
    {514, 0x02}, {515, 0x1D}, {516, 0x81}, {517, 0x10}, {518, 0x01}, {519, 0xA0},
    {521, 0x80}, {522, 0x12}, {523, 0x88}, {536, 0xFD}, {537, 0x1F}, {538, 0xFF},
    {539, 0xFF}, {543, 0x3F}, {555, 0x01}, {560, 0x85}, {561, 0xB6}, {562, 0x7B},
    {563, 0x73}, {568, 0x1B}, {569, 0xD9}, {570, 0xE3}, {571, 0x6C}, {572, 0x41},
    {573, 0xF9}, {574, 0xEF}, {575, 0xFE}, {576, 0x5F}, {577, 0xFF}, {578, 0x9A},
    {579, 0xC9}, {580, 0xFF}, {581, 0xFF}, {582, 0xFF}, {583, 0xF7}, {1523, 0x01},
    {1531, 0x03}, {1539, 0x02}, {1547, 0x36}, {1563, 0x0F}, {1570, 0x48}, {1571, 0x7F},
    {1579, 0x01}, {1923, 0x01}, {2033, 0x14}, {2034, 0x62}, {2035, 0x9F}, {2036, 0x08},
    {2037, 0xFF}, {2038, 0xD0}, {2044, 0x18}, {2045, 0x81}, {2047, 0x01}, {2048, 0x17},
    {2049, 0x05}, {2050, 0x0A}, {2051, 0x80}, {2052, 0x0A}, {2053, 0xA0}, {2054, 0x01},
    {2056, 0x81}, {2057, 0x64}, {2058, 0x42}, {2059, 0x40}, {2074, 0x10}, {2076, 0x02},
    {2099, 0x08}, {2101, 0x10}, {2102, 0x11}, {2103, 0x3D}, {2105, 0x02}, {2106, 0x72},
    {2107, 0x48}, {2108, 0x01}, {2109, 0x08}, {2112, 0x80}, {2123, 0x10}, {2272, 0x20},
    {2373, 0x01}, {2374, 0xD4}, {2375, 0xC0},
};

static_assert(sizeof(dSv_save_c) == 0x958);
static_assert(
    kIntroSkipSaveBytes[sizeof(kIntroSkipSaveBytes) / sizeof(kIntroSkipSaveBytes[0]) - 1]
        .offset < sizeof(dSv_save_c));

constexpr char kIntroSkipStage[] = "F_SP108";
constexpr s16 kIntroSkipPoint = 0;  // The preset's last-stay spawn ID is a separate field.
constexpr s8 kIntroSkipRoom = 0;
bool g_introSkipPending = false;

struct DataNewRestore {
    dFile_select_c* fileSelect = nullptr;
    u8 slot = 0xff;
    u8 value = 0;
};

DataNewRestore g_dataNewRestore{};

bool intro_skip_enabled() {
    int64_t value = kNewSaveModeDefault;
    return g_newSaveMode != 0 &&
        svc_config->get_int(mod_ctx, g_newSaveMode, &value) == MOD_OK && value == 1;
}

bool apply_intro_skip_preset(dSv_save_c* save) {
    if (save == nullptr) {
        return false;
    }

    auto& info = save->getPlayer().getPlayerInfo();
    std::array<char, sizeof(info.mPlayerName)> playerName{};
    std::array<char, sizeof(info.mHorseName)> horseName{};
    std::memcpy(playerName.data(), info.mPlayerName, playerName.size());
    std::memcpy(horseName.data(), info.mHorseName, horseName.size());
    auto* saveBytes = reinterpret_cast<u8*>(save);
    // The table contains the reference save's nonzero bytes.
    std::memset(saveBytes, 0, sizeof(*save));
    for (const SaveByte& byte : kIntroSkipSaveBytes) {
        saveBytes[byte.offset] = byte.value;
    }
    auto& newInfo = save->getPlayer().getPlayerInfo();
    std::memcpy(newInfo.mPlayerName, playerName.data(), playerName.size());
    std::memcpy(newInfo.mHorseName, horseName.data(), horseName.size());
    newInfo.setTotalTime(0);
    save->getPlayer().getPlayerStatusA().setRupee(150);
    return true;
}

void prepare_intro_skip_start() {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (save == nullptr) {
        return;
    }
    if (!dComIfGs_isEventBit(dSv_event_flag_c::F_0226)) {
        dComIfGs_offSaveSwitch(dStage_SaveTbl_FARON, 12);
    }
    dComIfGs_onSaveSwitch(dStage_SaveTbl_FARON, 20);
    save->getPlayer().getPlayerReturnPlace().set(kIntroSkipStage, kIntroSkipRoom, 0);
}

}  // namespace

void after_name_input2(ModContext*, void* args, void*, void*) {
    if (args == nullptr) {
        return;
    }
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    if (fileSelect == nullptr || !fileSelect->mIsSelectEnd ||
        fileSelect->mDataSelProc != dFile_select_c::DATASELPROC_NEXT_MODE_WAIT)
    {
        return;
    }

    g_introSkipPending = intro_skip_enabled() &&
        apply_intro_skip_preset(dComIfGs_getSaveData());
    if (g_introSkipPending) {
        svc_log->info(mod_ctx, "New Save Mode: Skip Intro preset applied");
    }
}

HookAction before_name_scene_change(ModContext*, void* args, void*, void*) {
    if (args == nullptr || !g_introSkipPending || g_dataNewRestore.fileSelect != nullptr) {
        return HOOK_CONTINUE;
    }

    prepare_intro_skip_start();
    auto* nameScene = mods::arg<dScnName_c*>(args, 0);
    auto* fileSelect = nameScene == nullptr ? nullptr : nameScene->dFs_c;
    if (fileSelect == nullptr || fileSelect->mSelectNum >= 3) {
        return HOOK_CONTINUE;
    }

    const u8 slot = fileSelect->mSelectNum;
    g_dataNewRestore = {fileSelect, slot, fileSelect->mIsDataNew[slot]};
    fileSelect->mIsDataNew[slot] = 0;
    return HOOK_CONTINUE;
}

void after_name_scene_change(ModContext*, void*, void*, void*) {
    if (g_dataNewRestore.fileSelect != nullptr && g_dataNewRestore.slot < 3) {
        g_dataNewRestore.fileSelect->mIsDataNew[g_dataNewRestore.slot] =
            g_dataNewRestore.value;
        g_dataNewRestore = {};
    }
}

HookAction before_set_next_stage(ModContext*, void* args, void*, void*) {
    if (args == nullptr || !g_introSkipPending) {
        return HOOK_CONTINUE;
    }

    const char* stage = mods::arg<const char*>(args, 0);
    const s16 point = mods::arg<s16>(args, 1);
    const s8 room = mods::arg<s8>(args, 2);
    const s8 layer = mods::arg<s8>(args, 3);
    if (stage == nullptr || std::strcmp(stage, "F_SP108") != 0 || point != 21 ||
        room != 1 || layer != 13)
    {
        return HOOK_CONTINUE;
    }

    prepare_intro_skip_start();
    mods::arg_ref<const char*>(args, 0) = kIntroSkipStage;
    mods::arg_ref<s16>(args, 1) = kIntroSkipPoint;
    mods::arg_ref<s8>(args, 2) = kIntroSkipRoom;
    mods::arg_ref<s8>(args, 3) = -1;
    g_introSkipPending = false;
    return HOOK_CONTINUE;
}

}  // namespace new_save_modes
