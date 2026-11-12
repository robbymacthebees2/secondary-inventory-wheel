

#include "mod.hpp"
#include "models.hpp"
#include "net/messages.hpp"
#include "print.hpp"
#include "util.hpp"

#include "mods/api.h"
#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/host.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/overlay.h"
#include "mods/svc/resource.h"
#include "mods/svc/texture.h"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_horse.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/J3DGraphBase/J3DTexture.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_OSX) ||     (defined(__linux__) && !defined(__ANDROID__))
#define COOP_HAS_FILE_MANAGER 1
#else
#define COOP_HAS_FILE_MANAGER 0
#endif

namespace {

const bool kCanOpenFolder = COOP_HAS_FILE_MANAGER != 0;

const char* const kPartFile[kSkinPartCount] = {
    "body.bmd", "face.bmd", "head.bmd", "hands.bmd", "sword.bmd", "shield.bmd"};

const char* const kOutfitDir[kSkinOutfitCount] = {
    "", "ordon", "zora", "magic", "wolf", "sumo"};

struct Skin {
    std::string name;

    std::string title;
    std::string author;
    std::string about;
    std::string path;
    bool has[kSkinOutfitCount][kSkinPartCount] = {};

    bool hasEquipment = false;
    bool hasCutscenes = false;
    bool hasVoice = false;
    bool baseWolf = false;
    uint32_t hash = 0;

    bool builtin = false;

    bool hidden = false;
    bool presetEnabled = true;

    bool silentVoice = false;
};

std::vector<Skin> s_skins;
bool s_scanned = false;

struct LoadedPart {
    std::string skin;
    int outfit = 0;
    int part = 0;
    bool forLink = false;
    J3DModelData* data = nullptr;
    bool failed = false;
};
std::vector<LoadedPart> s_loadedParts;

struct LoadedCutscene {
    std::string skin;
    std::string file;
    J3DModelData* data = nullptr;
    bool failed = false;
};
std::vector<LoadedCutscene> s_loadedCutscenes;

const char* const kSlotVarName[kSkinChoiceCount] = {
    "model_hero", "model_sumo", "model_ordon", "model_zora", "model_magic",
    "model_wolf", "model_equipment", "model_cutscenes", "model_voice",
    "model_sword_wood", "model_sword_ordon", "model_sword_master",
    "model_shield_hylian", "model_shield_ordon",
    "model_hookshot", "model_boomerang", "model_bombs", "model_bow", "model_slingshot",
    "model_ball_chain", "model_spinner", "model_fishing_rod", "model_bottle",
};
const char* const kSlotLabel[kSkinChoiceCount] = {
    "Hero's clothes", "Sumo clothes", "Ordon clothes", "Zora armor", "Magic armor",
    "Wolf", "Equipment", "Cutscenes", "Voice",
    "Wooden Sword", "Ordon Sword", "Master Sword", "Hylian Shield", "Ordon Shield",
    "Clawshot", "Boomerang", "Bombs", "Bow", "Slingshot",
    "Ball and Chain", "Spinner", "Fishing Rod", "Bottle",
};

struct EquipSlotFile {
    const char* file;
    int slot;
};
const EquipSlotFile kEquipSlotFiles[] = {
    {"al_swb.bmd", kSkinChoiceWoodSword},
    {"al_swa.bmd", kSkinChoiceOrdonSword},
    {"al_poda.bmd", kSkinChoiceOrdonSword},
    {"al_swm.bmd", kSkinChoiceMasterSword},
    {"al_podm.bmd", kSkinChoiceMasterSword},
    {"o_al_swm.bmd", kSkinChoiceMasterSword},
    {"al_sha.bmd", kSkinChoiceHylianShield},

    {"al_shb.bmd", kSkinChoiceOrdonShield},
    {"al_shc.bmd", kSkinChoiceOrdonShield},

};
ConfigVarHandle s_slotVar[kSkinChoiceCount] = {};
ConfigVarHandle s_eponaSkinVar = 0;
ConfigVarHandle s_filesVar = 0;

bool models_dir(std::filesystem::path& out) {
    if (svc_host == nullptr || !SERVICE_HAS(svc_host, HostService, data_dir)) return false;
    const char* dir = nullptr;
    if (svc_host->data_dir(mod_ctx, &dir) != MOD_OK || dir == nullptr) return false;
    out = std::filesystem::u8path(dir) / "models";
    std::error_code ec;
    std::filesystem::create_directories(out, ec);
    return !ec;
}

void read_about(const std::filesystem::path& dir, Skin& skin) {
    std::ifstream file(dir / "model.txt");
    if (!file) return;
    std::string line;
    while (std::getline(file, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        const auto trim = [](std::string& t) {
            while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
            while (!t.empty() && (t.back() == ' ' || t.back() == '\t' || t.back() == '\r')) {
                t.pop_back();
            }
        };
        trim(key);
        trim(value);
        if (key == "name") skin.title = value;
        else if (key == "by" || key == "author") skin.author = value;
        else if (key == "about") skin.about = value;
        else if (key == "preset") skin.presetEnabled = value != "false";
        else if (key == "base") skin.baseWolf = value == "wolf";
    }
}

bool scan_one(const std::filesystem::path& path, Skin& skin) {
    skin.name = path_text(path.filename());
    skin.path = path_text(path);
    read_about(path, skin);
    if (skin.name.size() >= kSkinNameMax) skin.name.resize(kSkinNameMax - 1);
    bool any = false;
    if (skin.baseWolf) {
        skin.has[kSkinOutfitWolf][kSkinPartBody] = true;
        any = true;
    }
    for (int o = 0; o < kSkinOutfitCount; ++o) {
        const std::filesystem::path dir =
            kOutfitDir[o][0] == '\0' ? path : path / kOutfitDir[o];
        for (int i = 0; i < kSkinPartCount; ++i) {
            std::error_code fec;
            const auto size = std::filesystem::file_size(dir / kPartFile[i], fec);
            if (fec) continue;
            skin.has[o][i] = true;
            any = true;

            skin.hash = skin.hash * 31u + static_cast<uint32_t>(size) +
                        static_cast<uint32_t>(i * 8 + o);
        }
    }
    std::error_code dec;
    skin.hasEquipment = is_directory_ci(path / "equipment", dec) && !dec &&
        !std::filesystem::is_empty(path / "equipment", dec);
    skin.hasCutscenes = is_directory_ci(path / "cutscene", dec) && !dec &&
        !std::filesystem::is_empty(path / "cutscene", dec);
    skin.hasVoice = exists_ci(path / "voices.bin", dec) && !dec;

    return any || skin.hasEquipment || skin.hasCutscenes || skin.hasVoice;
}

void scan_dir(const std::filesystem::path& dir, int& count) {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path_ci(dir), ec)) {
        if (!entry.is_directory()) continue;
        Skin skin;
        if (!scan_one(entry.path(), skin)) continue;

        bool taken = false;
        for (const Skin& have : s_skins) taken = taken || have.name == skin.name;
        if (taken) continue;
        s_skins.push_back(std::move(skin));
        ++count;
    }
}

const char* const kByGame = "Game";

void add_builtin(const char* name, const char* author, const char* about, int first, int last,
    bool equipment, uint32_t hash, bool silentVoice, bool hidden) {
    Skin skin;
    skin.name = name;
    skin.title = name;
    skin.author = author;
    skin.about = about;
    skin.builtin = true;
    for (int o = first; o <= last; ++o) skin.has[o][kSkinPartBody] = true;
    skin.hasEquipment = equipment;
    skin.hash = hash;
    skin.silentVoice = silentVoice;
    skin.hidden = hidden;
    s_skins.push_back(skin);
}

void rescan() {
    s_skins.clear();
    s_scanned = true;
    int found = 0;

    add_builtin(kGanondorfSkin, kByGame, "", kSkinOutfitHero, kSkinOutfitMagic, true, 0x6A4E0D0Fu, true, false);

    add_builtin(kBeastGanonSkin, kByGame, "", kSkinOutfitWolf, kSkinOutfitWolf, false, 0x6A4E0D10u, false, true);

    add_builtin(kHerosShadeSkin, kByGame, "", kSkinOutfitHero, kSkinOutfitWolf, true, 0x6A4E0D11u, true, false);
    std::filesystem::path dir;
    if (models_dir(dir)) scan_dir(dir, found);
    const auto preset_order = [](const Skin& skin) {
        if (skin.name == "Linkle") return 0;
        if (skin.name == "Twilight Wolf") return 1;
        if (skin.name == "Concept Art Wolf") return 2;
        return 3;
    };
    std::sort(s_skins.begin(), s_skins.end(), [&](const Skin& a, const Skin& b) {
        const int aOrder = preset_order(a);
        const int bOrder = preset_order(b);
        return aOrder != bOrder ? aOrder < bOrder : a.name < b.name;
    });
    coop_log::info("coop_mod: [SKIN] {} model(s) in {}", s_skins.size(), path_text(dir));
}

const Skin* find(const char* name) {
    if (name == nullptr || name[0] == '\0') return nullptr;
    if (!s_scanned) rescan();
    for (const Skin& s : s_skins) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

void forget_failures() {
    for (size_t i = 0; i < s_loadedParts.size();) {
        if (s_loadedParts[i].failed) {
            s_loadedParts.erase(s_loadedParts.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
}

}

std::vector<OverlayHandle> s_overlays;

void clear_overlays() {
    if (svc_overlay == nullptr) return;
    for (OverlayHandle h : s_overlays) svc_overlay->remove(mod_ctx, h);
    s_overlays.clear();
}

void apply_overlays() {
    clear_overlays();
}

void unpack_shipped_models();

ConfigVarHandle s_cycleVar = 0;
ConfigVarHandle s_outfitCycleVar = 0;
ConfigVarHandle s_noLocalSkinVar = 0;

namespace {

DEFINE_HOOK(&daHorse_c::draw, EponaDraw);

const char* const kOotStyleEponaName = "OOT Style Epona";
const char* const kTwilightEponaName = "Twilight Epona";
const char* const kNightwalkerEponaName = "Nightwalker Epona";

const char* const kOotStyleEponaTexturePaths[] = {
    "epona_skins/OOT Style Epona/Eyes/tex1_32x32_1b47c467fc5ca5bc_14.dds",
    "epona_skins/OOT Style Epona/Eyes/tex1_32x32_24b0258ccf8eed45_14.dds",
    "epona_skins/OOT Style Epona/Eyes/tex1_32x32_b9f4dbd94bc5465b_14.dds",
    "epona_skins/OOT Style Epona/Eyes/tex1_32x32_ec429699463fddb2_14.dds",
    "epona_skins/OOT Style Epona/Reins/tex1_16x16_94934198a2bc6db8_14.png",
    "epona_skins/OOT Style Epona/tex1_128x128_e1b40d4e5b4720b1_69978ebdfb7b19af_9.dds",
    "epona_skins/OOT Style Epona/tex1_256x256_f473331d6d5b148d_14.dds",
    "epona_skins/OOT Style Epona/tex1_64x128_7e0285365dbe9413_968b4d3e0cf3ab18_9.dds",
    "epona_skins/OOT Style Epona/tex1_64x128_e8fde3c46a49cb15_e133b5a9f7e6da15_9.dds",
};

const char* const kTwilightEponaTexturePaths[] = {
    "epona_skins/Twilight Epona/tex1_128x128_e1b40d4e5b4720b1_69978ebdfb7b19af_9.dds",
    "epona_skins/Twilight Epona/tex1_256x256_f473331d6d5b148d_14.dds",
    "epona_skins/Twilight Epona/tex1_32x32_1b47c467fc5ca5bc_14.dds",
    "epona_skins/Twilight Epona/tex1_32x32_24b0258ccf8eed45_14.dds",
    "epona_skins/Twilight Epona/tex1_32x32_b9f4dbd94bc5465b_14.dds",
    "epona_skins/Twilight Epona/tex1_32x32_ec429699463fddb2_14.dds",
    "epona_skins/Twilight Epona/tex1_64x128_e8fde3c46a49cb15_e133b5a9f7e6da15_9.dds",
};

bool s_eponaTextureServiceWarningLogged = false;
bool s_eponaResourceServiceWarningLogged = false;

struct EponaTextureOwner {
    fopAc_ac_c* actor = nullptr;
    uint32_t actorId = 0;
    J3DModelData* modelData = nullptr;
    std::string skin;
    std::vector<TextureReplacementHandle> handles;
    J3DModel* originalModel = nullptr;
    J3DModelData* originalModelData = nullptr;
    bool modelSwapped = false;
    bool failed = false;
};

std::array<EponaTextureOwner, kCoopMaxPlayers> s_eponaTextureOwners{};

constexpr size_t kMaxNightwalkerBmdSize = 8 * 1024 * 1024;
std::unique_ptr<u8[]> s_nightwalkerHeapStorage;
JKRExpHeap* s_nightwalkerHeap = nullptr;
J3DModelData* s_nightwalkerModelData = nullptr;
std::array<J3DModel*, kCoopMaxPlayers> s_nightwalkerModels{};
bool s_nightwalkerLoadAttempted = false;
bool s_nightwalkerCompatibilityErrorLogged = false;

struct EponaHeapScope {
    JKRHeap* previous;
    explicit EponaHeapScope(JKRHeap* heap) : previous(heap->becomeCurrentHeap()) {}
    ~EponaHeapScope() {
        if (previous != nullptr) previous->becomeCurrentHeap();
    }
};

bool load_nightwalker_model() {
    if (s_nightwalkerModelData != nullptr) return true;
    if (s_nightwalkerLoadAttempted) return false;
    s_nightwalkerLoadAttempted = true;

    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    const char* const path = "epona_skins/Nightwalker Epona/hs.bmd";
    if (svc_resource == nullptr ||
        svc_resource->load(mod_ctx, path, &buffer) != MOD_OK ||
        buffer.data == nullptr || buffer.size < 32 ||
        buffer.size > kMaxNightwalkerBmdSize)
    {
        if (buffer.data != nullptr) svc_resource->free(mod_ctx, &buffer);
        coop_log::warn("coop_mod: [SKIN] failed to load Nightwalker Epona model");
        return false;
    }

    const u32 heapSize = 8 * 1024 * 1024 + static_cast<u32>(buffer.size) * 4;
    s_nightwalkerHeapStorage.reset(new (std::nothrow) u8[heapSize + 31]);
    if (!s_nightwalkerHeapStorage) {
        svc_resource->free(mod_ctx, &buffer);
        coop_log::warn("coop_mod: [SKIN] could not allocate Nightwalker Epona model memory");
        return false;
    }
    void* memory = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(s_nightwalkerHeapStorage.get()) + 31) &
        ~uintptr_t{31});
    s_nightwalkerHeap = JKRExpHeap::create(
        memory, heapSize, JKRHeap::getRootHeap(), false);
    if (s_nightwalkerHeap == nullptr) {
        svc_resource->free(mod_ctx, &buffer);
        s_nightwalkerHeapStorage.reset();
        coop_log::warn("coop_mod: [SKIN] could not create Nightwalker Epona model heap");
        return false;
    }

    bool loaded = false;
    {
        EponaHeapScope heapScope(s_nightwalkerHeap);
        auto* bytes = static_cast<u8*>(s_nightwalkerHeap->alloc(
            static_cast<u32>(buffer.size), 32));
        if (bytes != nullptr) {
            std::memcpy(bytes, buffer.data, buffer.size);
            J3DModelData* data = J3DModelLoaderDataBase::load(bytes, 0x59040030);
            if (data != nullptr && data->getJointNum() == 38 &&
                data->getMaterialNum() == 6 && data->getShapeNum() == 6)
            {
                bool materialsReady = true;
                for (u16 i = 0; i < data->getMaterialNum(); ++i) {
                    J3DMaterial* material = data->getMaterialNodePointer(i);
                    if (material == nullptr) {
                        materialsReady = false;
                        break;
                    }
                    material->change();
                    auto* animation = JKR_NEW J3DMaterialAnm();
                    if (animation == nullptr) {
                        materialsReady = false;
                        break;
                    }
                    material->setMaterialAnm(animation);
                }
                if (materialsReady &&
                    data->newSharedDisplayList(J3DMdlFlag_UseSingleDL) ==
                        kJ3DError_Success)
                {
                    data->simpleCalcMaterial(const_cast<MtxP>(j3dDefaultMtx));
                    data->makeSharedDL();
                    s_nightwalkerModelData = data;
                    loaded = true;
                }
            }
        }
    }
    svc_resource->free(mod_ctx, &buffer);
    if (!loaded) {
        coop_log::warn("coop_mod: [SKIN] Nightwalker Epona model data is invalid");
    }
    return loaded;
}

bool swap_nightwalker_model(daHorse_c* horse, EponaTextureOwner& owner, uint8_t player) {
    if (horse == nullptr || horse->m_model == nullptr ||
        horse->m_modelData == nullptr || player >= kCoopMaxPlayers ||
        !load_nightwalker_model())
    {
        return false;
    }
    if (horse->m_modelData->getJointNum() != s_nightwalkerModelData->getJointNum() ||
        horse->m_modelData->getMaterialNum() != s_nightwalkerModelData->getMaterialNum() ||
        horse->m_modelData->getShapeNum() != s_nightwalkerModelData->getShapeNum())
    {
        if (!s_nightwalkerCompatibilityErrorLogged) {
            coop_log::warn(
                "coop_mod: [SKIN] Nightwalker Epona rig does not match the active horse model");
            s_nightwalkerCompatibilityErrorLogged = true;
        }
        return false;
    }

    J3DModel*& nightwalkerModel = s_nightwalkerModels[player];
    if (nightwalkerModel == nullptr) {
        EponaHeapScope heapScope(s_nightwalkerHeap);
        nightwalkerModel =
            mDoExt_J3DModel__create(s_nightwalkerModelData, 0x80000, 0x11020084);
        if (nightwalkerModel == nullptr) return false;
    }

    J3DModel* const originalModel = horse->m_model;
    J3DModelData* const originalData = horse->m_modelData;
    for (u16 i = 0; i < s_nightwalkerModelData->getJointNum(); ++i) {
        J3DJoint* const originalJoint = originalData->getJointNodePointer(i);
        J3DJoint* const nightwalkerJoint = s_nightwalkerModelData->getJointNodePointer(i);
        if (originalJoint == nullptr || nightwalkerJoint == nullptr) return false;
    }
    for (u16 i = 0; i < s_nightwalkerModelData->getJointNum(); ++i) {
        J3DJoint* const originalJoint = originalData->getJointNodePointer(i);
        J3DJoint* const nightwalkerJoint = s_nightwalkerModelData->getJointNodePointer(i);
        nightwalkerJoint->setCallBack(originalJoint->getCallBack());
        nightwalkerJoint->setMtxCalc(originalJoint->getMtxCalc());
    }
    for (u16 i = 0; i < s_nightwalkerModelData->getShapeNum(); ++i) {
        J3DShape* const originalShape = originalData->getShapeNodePointer(i);
        J3DShape* const nightwalkerShape = s_nightwalkerModelData->getShapeNodePointer(i);
        if (originalShape == nullptr || nightwalkerShape == nullptr) return false;
        if (originalShape->checkFlag(J3DShpFlag_Visible)) {
            nightwalkerShape->hide();
        } else {
            nightwalkerShape->show();
        }
    }
    nightwalkerModel->setBaseTRMtx(originalModel->getBaseTRMtx());
    nightwalkerModel->setBaseScale(*originalModel->getBaseScale());
    nightwalkerModel->setUserArea(reinterpret_cast<uintptr_t>(horse));
    owner.originalModel = originalModel;
    owner.originalModelData = horse->m_modelData;
    owner.modelSwapped = true;
    horse->m_model = nightwalkerModel;
    horse->m_modelData = s_nightwalkerModelData;
    nightwalkerModel->calc();
    return true;
}

uint64_t xxh64_round(uint64_t accumulator, uint64_t input) {
    accumulator += input * 14029467366897019727ull;
    accumulator = (accumulator << 31) | (accumulator >> 33);
    return accumulator * 11400714785074694791ull;
}

uint64_t xxh64_merge_round(uint64_t accumulator, uint64_t value) {
    accumulator ^= xxh64_round(0, value);
    return accumulator * 11400714785074694791ull + 9650029242287828579ull;
}

uint64_t xxh64(const uint8_t* data, size_t size) {
    constexpr uint64_t p1 = 11400714785074694791ull;
    constexpr uint64_t p2 = 14029467366897019727ull;
    constexpr uint64_t p3 = 1609587929392839161ull;
    constexpr uint64_t p4 = 9650029242287828579ull;
    constexpr uint64_t p5 = 2870177450012600261ull;
    const uint8_t* cursor = data;
    const uint8_t* const end = data + size;
    uint64_t hash = p5;
    if (size >= 32) {
        const uint8_t* const limit = end - 32;
        uint64_t v1 = p1 + p2;
        uint64_t v2 = p2;
        uint64_t v3 = 0;
        uint64_t v4 = 0 - p1;
        do {
            uint64_t value = 0;
            std::memcpy(&value, cursor, sizeof(value));
            v1 = xxh64_round(v1, value);
            std::memcpy(&value, cursor + 8, sizeof(value));
            v2 = xxh64_round(v2, value);
            std::memcpy(&value, cursor + 16, sizeof(value));
            v3 = xxh64_round(v3, value);
            std::memcpy(&value, cursor + 24, sizeof(value));
            v4 = xxh64_round(v4, value);
            cursor += 32;
        } while (cursor <= limit);
        hash = ((v1 << 1) | (v1 >> 63)) + ((v2 << 7) | (v2 >> 57)) +
               ((v3 << 12) | (v3 >> 52)) + ((v4 << 18) | (v4 >> 46));
        hash = xxh64_merge_round(hash, v1);
        hash = xxh64_merge_round(hash, v2);
        hash = xxh64_merge_round(hash, v3);
        hash = xxh64_merge_round(hash, v4);
    }

    hash += size;
    while (cursor + 8 <= end) {
        uint64_t value = 0;
        std::memcpy(&value, cursor, sizeof(value));
        hash ^= xxh64_round(0, value);
        hash = ((hash << 27) | (hash >> 37)) * p1 + p4;
        cursor += 8;
    }
    if (cursor + 4 <= end) {
        uint32_t value = 0;
        std::memcpy(&value, cursor, sizeof(value));
        hash ^= static_cast<uint64_t>(value) * p1;
        hash = ((hash << 23) | (hash >> 41)) * p2 + p3;
        cursor += 4;
    }
    while (cursor < end) {
        hash ^= static_cast<uint64_t>(*cursor++) * p5;
        hash = ((hash << 11) | (hash >> 53)) * p1;
    }
    hash ^= hash >> 33;
    hash *= p2;
    hash ^= hash >> 29;
    hash *= p3;
    hash ^= hash >> 32;
    return hash;
}

size_t epona_texture_source_size(uint32_t format, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return 0;
    const auto div_ceil = [](uint32_t value, uint32_t divisor) {
        return (value + divisor - 1) / divisor;
    };
    switch (format) {
    case GX_TF_I4:
    case GX_TF_C4:
    case GX_TF_CMPR:
        return static_cast<size_t>(div_ceil(width, 8)) * div_ceil(height, 8) * 32;
    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_TF_C8:
        return static_cast<size_t>(div_ceil(width, 8)) * div_ceil(height, 4) * 32;
    case GX_TF_IA8:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
    case GX_TF_C14X2:
        return static_cast<size_t>(div_ceil(width, 4)) * div_ceil(height, 4) * 32;
    case GX_TF_RGBA8:
        return static_cast<size_t>(div_ceil(width, 4)) * div_ceil(height, 4) * 64;
    default:
        return 0;
    }
}

size_t epona_rgba8_size(uint32_t width, uint32_t height) {
    return static_cast<size_t>((width + 3) / 4) * ((height + 3) / 4) * 64;
}

struct EponaTextureSource {
    uint64_t hash = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;
};

bool parse_epona_texture_source(const char* path, EponaTextureSource& out) {
    if (path == nullptr) return false;
    const std::string file(path);
    const size_t slash = file.find_last_of("/\\");
    const size_t nameStart = slash == std::string::npos ? 0 : slash + 1;
    if (file.compare(nameStart, 5, "tex1_") != 0) return false;
    const size_t x = file.find('x', nameStart + 5);
    const size_t dimensionEnd =
        x == std::string::npos ? x : file.find('_', x + 1);
    const size_t hashStart =
        dimensionEnd == std::string::npos ? dimensionEnd : dimensionEnd + 1;
    const size_t hashEnd = file.find('_', hashStart);
    const size_t formatStart = file.find_last_of('_');
    const size_t extension = file.find_last_of('.');
    if (x == std::string::npos || dimensionEnd == std::string::npos ||
        hashEnd == std::string::npos || formatStart == std::string::npos ||
        extension == std::string::npos || formatStart < hashEnd ||
        extension <= formatStart + 1)
    {
        return false;
    }

    char* end = nullptr;
    const std::string widthText = file.substr(nameStart + 5, x - nameStart - 5);
    const unsigned long width = std::strtoul(widthText.c_str(), &end, 10);
    if (end == widthText.c_str() || *end != '\0') return false;
    const std::string heightText = file.substr(x + 1, dimensionEnd - x - 1);
    const unsigned long height = std::strtoul(heightText.c_str(), &end, 10);
    if (end == heightText.c_str() || *end != '\0') return false;
    const std::string hashText = file.substr(hashStart, hashEnd - hashStart);
    const uint64_t hash = std::strtoull(hashText.c_str(), &end, 16);
    if (end == hashText.c_str() || *end != '\0') return false;
    const std::string formatText =
        file.substr(formatStart + 1, extension - formatStart - 1);
    const unsigned long format = std::strtoul(formatText.c_str(), &end, 10);
    if (end == formatText.c_str() || *end != '\0' || width == 0 || height == 0) {
        return false;
    }

    out = {
        .hash = hash,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .format = static_cast<uint32_t>(format),
    };
    return true;
}

std::string epona_gx_texture_path(const char* sourcePath) {
    std::string path = sourcePath != nullptr ? sourcePath : "";
    const size_t extension = path.find_last_of('.');
    if (extension != std::string::npos) path.replace(extension, std::string::npos, ".gxrgba");
    return path;
}

std::string epona_skin_for_player(uint8_t player) {
    if (player == coop_net_local_id()) return skins_local_epona_skin();
    const CoopPeer& peer = features_peer_of(player);
    if (!peer.present) return "";
    if (std::strncmp(peer.eponaSkin, kOotStyleEponaName, kSkinNameMax) == 0) {
        return kOotStyleEponaName;
    }
    if (std::strncmp(peer.eponaSkin, kTwilightEponaName, kSkinNameMax) == 0) {
        return kTwilightEponaName;
    }
    if (std::strncmp(peer.eponaSkin, kNightwalkerEponaName, kSkinNameMax) == 0) {
        return kNightwalkerEponaName;
    }
    return "";
}

void unregister_epona_owner(EponaTextureOwner& owner) {
    if (svc_texture != nullptr) {
        for (TextureReplacementHandle handle : owner.handles) {
            if (svc_texture->unregister(mod_ctx, handle) != MOD_OK) {
                coop_log::warn("coop_mod: [SKIN] failed to unregister an Epona texture");
            }
        }
    }
    owner.handles.clear();
}

struct EponaTextureAsset {
    std::string path;
    std::vector<uint8_t> data;
    uint32_t width = 0;
    uint32_t height = 0;
    bool loaded = false;
    bool failed = false;
};

std::array<EponaTextureAsset, 16> s_eponaTextureAssets{};

EponaTextureAsset* epona_texture_asset(const char* path) {
    if (path == nullptr) return nullptr;
    EponaTextureAsset* asset = nullptr;
    for (EponaTextureAsset& candidate : s_eponaTextureAssets) {
        if (candidate.path == path) return &candidate;
        if (asset == nullptr && candidate.path.empty()) asset = &candidate;
    }
    if (asset == nullptr) {
        coop_log::warn("coop_mod: [SKIN] Epona texture asset cache is full");
        return nullptr;
    }
    asset->path = path;
    if (svc_resource == nullptr) {
        if (!s_eponaResourceServiceWarningLogged) {
            coop_log::warn(
                "coop_mod: [SKIN] resource service unavailable; Epona textures cannot be loaded");
            s_eponaResourceServiceWarningLogged = true;
        }
        asset->failed = true;
        return nullptr;
    }
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    if (svc_resource->load(mod_ctx, path, &buffer) != MOD_OK ||
        buffer.data == nullptr || buffer.size < 8)
    {
        if (buffer.data != nullptr) svc_resource->free(mod_ctx, &buffer);
        asset->failed = true;
        coop_log::warn("coop_mod: [SKIN] failed to load converted Epona texture '{}'", path);
        return nullptr;
    }

    std::memcpy(&asset->width, buffer.data, sizeof(asset->width));
    std::memcpy(
        &asset->height,
        static_cast<const uint8_t*>(buffer.data) + sizeof(asset->width),
        sizeof(asset->height));
    const size_t dataSize = buffer.size - 8;
    if (asset->width == 0 || asset->height == 0 ||
        dataSize != epona_rgba8_size(asset->width, asset->height))
    {
        svc_resource->free(mod_ctx, &buffer);
        asset->failed = true;
        coop_log::warn(
            "coop_mod: [SKIN] converted Epona texture '{}' has invalid dimensions", path);
        return nullptr;
    }
    const auto* data = static_cast<const uint8_t*>(buffer.data) + 8;
    asset->data.assign(data, data + dataSize);
    svc_resource->free(mod_ctx, &buffer);
    asset->loaded = true;
    s_eponaResourceServiceWarningLogged = false;
    return asset;
}

void register_epona_owner(EponaTextureOwner& owner) {
    if (owner.skin.empty() || owner.failed || !owner.handles.empty()) return;
    const char* const* paths = nullptr;
    size_t count = 0;
    if (owner.skin == kOotStyleEponaName) {
        paths = kOotStyleEponaTexturePaths;
        count = sizeof(kOotStyleEponaTexturePaths) / sizeof(kOotStyleEponaTexturePaths[0]);
    } else if (owner.skin == kTwilightEponaName) {
        paths = kTwilightEponaTexturePaths;
        count = sizeof(kTwilightEponaTexturePaths) / sizeof(kTwilightEponaTexturePaths[0]);
    } else {
        return;
    }

    if (owner.actor == nullptr || owner.modelData == nullptr ||
        owner.modelData->getTexture() == nullptr || svc_texture == nullptr ||
        svc_resource == nullptr)
    {
        return;
    }
    J3DTexture* textures = owner.modelData->getTexture();
    std::vector<const void*> registeredPointers;
    bool failed = false;
    for (u16 i = 0; i < textures->getNum(); ++i) {
        ResTIMG* timg = textures->getResTIMG(i);
        if (timg == nullptr) continue;
        const uint32_t width = static_cast<uint16_t>(timg->width);
        const uint32_t height = static_cast<uint16_t>(timg->height);
        const uint32_t format = timg->format;
        const size_t sourceSize = epona_texture_source_size(format, width, height);
        if (sourceSize == 0) continue;
        const void* imageData = textures->getImgDataPtr(i);
        if (imageData == nullptr) continue;
        const uint64_t sourceHash =
            xxh64(static_cast<const uint8_t*>(imageData), sourceSize);
        const char* replacementPath = nullptr;
        for (size_t pathIndex = 0; pathIndex < count; ++pathIndex) {
            EponaTextureSource source;
            if (parse_epona_texture_source(paths[pathIndex], source) &&
                source.hash == sourceHash && source.width == width &&
                source.height == height && source.format == format)
            {
                replacementPath = paths[pathIndex];
                break;
            }
        }
        if (replacementPath == nullptr ||
            std::find(registeredPointers.begin(), registeredPointers.end(), imageData) !=
                registeredPointers.end())
        {
            continue;
        }

        const std::string dataPath = epona_gx_texture_path(replacementPath);
        EponaTextureAsset* replacementAsset =
            epona_texture_asset(dataPath.c_str());
        if (replacementAsset == nullptr) {
            failed = true;
            break;
        }

        TextureKey key = TEXTURE_KEY_INIT;
        key.kind = TEXTURE_KEY_POINTER;
        key.pointer = imageData;
        key.width = width;
        key.height = height;
        key.gx_format = format;
        TextureData replacement = TEXTURE_DATA_INIT;
        replacement.data = replacementAsset->data.data();
        replacement.size = replacementAsset->data.size();
        replacement.width = replacementAsset->width;
        replacement.height = replacementAsset->height;
        replacement.mip_count = 1;
        replacement.gx_format = GX_TF_RGBA8;
        TextureReplacementHandle handle = 0;
        const ModResult result =
            svc_texture->register_data(mod_ctx, &key, &replacement, &handle);
        if (result != MOD_OK) {
            coop_log::warn(
                "coop_mod: [SKIN] failed to register Epona texture '{}' ({})",
                dataPath, static_cast<int>(result));
            failed = true;
            break;
        }
        owner.handles.push_back(handle);
        registeredPointers.push_back(imageData);
    }

    if (failed) {
        unregister_epona_owner(owner);
        owner.failed = true;
    } else if (owner.handles.empty()) {
        coop_log::warn(
            "coop_mod: [SKIN] no source textures matched the selected Epona skin");
        owner.failed = true;
    }
}

void update_epona_textures() {
    if (svc_texture == nullptr) {
        if (!s_eponaTextureServiceWarningLogged) {
            coop_log::warn(
                "coop_mod: [SKIN] texture service unavailable; Epona textures cannot be loaded");
            s_eponaTextureServiceWarningLogged = true;
        }
        return;
    }
    s_eponaTextureServiceWarningLogged = false;

    std::array<fopAc_ac_c*, kCoopMaxPlayers> actors{};
    std::array<uint32_t, kCoopMaxPlayers> actorIds{};
    std::array<J3DModelData*, kCoopMaxPlayers> modelData{};
    std::array<std::string, kCoopMaxPlayers> skins{};
    const uint8_t localPlayer = coop_net_local_id();
    bool changed = false;
    for (uint8_t player = 0; player < kCoopMaxPlayers; ++player) {
        actors[player] = horses_actor_for(player);
        if (player == localPlayer && actors[player] == nullptr) {
            actors[player] = dComIfGp_getHorseActor();
        }
        actorIds[player] = actors[player] != nullptr ? fopAcM_GetID(actors[player]) : 0;
        auto* horse = static_cast<daHorse_c*>(actors[player]);
        modelData[player] = horse != nullptr ? horse->m_modelData : nullptr;
        skins[player] = epona_skin_for_player(player);
        const EponaTextureOwner& owner = s_eponaTextureOwners[player];
        changed = changed || owner.actor != actors[player] ||
                  owner.actorId != actorIds[player] ||
                  owner.modelData != modelData[player] || owner.skin != skins[player];
    }

    if (changed) {
        for (EponaTextureOwner& owner : s_eponaTextureOwners) {
            unregister_epona_owner(owner);
            owner = {};
        }
    }

    for (uint8_t player = 0; player < kCoopMaxPlayers; ++player) {
        EponaTextureOwner& owner = s_eponaTextureOwners[player];
        owner.actor = actors[player];
        owner.actorId = actorIds[player];
        owner.modelData = modelData[player];
        owner.skin = skins[player];
    }

}

HookAction prepare_epona_skin_draw(ModContext*, void* args, void*, void*) {
    auto* horse = mods::arg<daHorse_c*>(args, 0);
    if (horse == nullptr) return HOOK_CONTINUE;
    update_epona_textures();
    for (uint8_t player = 0; player < kCoopMaxPlayers; ++player) {
        EponaTextureOwner& owner = s_eponaTextureOwners[player];
        if (owner.actor == horse) {
            if (owner.skin == kNightwalkerEponaName) {
                swap_nightwalker_model(horse, owner, player);
            } else {
                register_epona_owner(owner);
            }
            break;
        }
    }
    return HOOK_CONTINUE;
}

void finish_epona_skin_draw(ModContext*, void* args, void*, void*) {
    auto* horse = mods::arg<daHorse_c*>(args, 0);
    if (horse == nullptr) return;
    for (EponaTextureOwner& owner : s_eponaTextureOwners) {
        if (owner.actor == horse) {
            if (owner.modelSwapped) {
                horse->m_model = owner.originalModel;
                horse->m_modelData = owner.originalModelData;
                owner.originalModel = nullptr;
                owner.originalModelData = nullptr;
                owner.modelSwapped = false;
            }
            break;
        }
    }
}

}

void skins_texture_replacements_update() {
    update_epona_textures();
}

void skins_cycle_update() {
    const int64_t period = cfg_int(s_cycleVar, 0);
    if (period <= 0) return;
    static int tick = 0;
    if (++tick < period) return;
    tick = 0;

    std::vector<std::string> presets;
    for (int i = 0; i < skins_count(); ++i) {
        const char* name = skins_name(i);
        if (skins_preset_enabled(name)) presets.emplace_back(name);
    }
    static int next = 0;
    if (next > static_cast<int>(presets.size())) next = 0;
    const std::string want =
        (next == 0) ? std::string("") : presets[static_cast<size_t>(next - 1)];
    next = (next + 1) % (static_cast<int>(presets.size()) + 1);
    coop_log::info("coop_mod: [SKIN-CYCLE] switching to '{}'", want.empty() ? "Default" : want);
    skins_set_local_all(want.c_str());
}

const int kOutfitSettleTicks = 90;

void skins_outfit_cycle_update() {
    const int64_t period = cfg_int(s_outfitCycleVar, 0);
    if (period <= 0) return;
    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (alink == nullptr) return;

    static int settle = 0;

    static daAlink_c* s_lastAlink = nullptr;
    if (alink != s_lastAlink || dComIfGp_isEnableNextStage() || alink->mClothesChangeWaitTimer != 0) {
        s_lastAlink = alink;
        settle = kOutfitSettleTicks;
        return;
    }
    if (settle > 0) {
        --settle;
        return;
    }

    static int tick = 0;
    if (++tick < period) return;
    tick = 0;
    settle = kOutfitSettleTicks;

    if (alink->checkWolf()) {
        coop_log::info("coop_mod: [OUTFIT-CYCLE] transforming back");
        coop_debug_force_transform();
        return;
    }

    static const u8 kWear[] = {
        dItemNo_WEAR_KOKIRI_e,
        dItemNo_WEAR_CASUAL_e,
        dItemNo_WEAR_ZORA_e,
    };
    const int wearCount = static_cast<int>(sizeof(kWear) / sizeof(kWear[0]));
    static int next = 0;
    if (next == wearCount) {
        next = 0;
        coop_log::info("coop_mod: [OUTFIT-CYCLE] transforming");
        coop_debug_force_transform();
        return;
    }
    const u8 want = kWear[next];
    ++next;
    coop_log::info("coop_mod: [OUTFIT-CYCLE] changing clothes to item {}", static_cast<int>(want));
    dComIfGs_setSelectEquipClothes(want);
    alink->setClothesChange(0);
}

void skins_init() {
    models_warp_guard_init();
    unpack_shipped_models();
    if (mods::hook::add_pre<EponaDraw>(prepare_epona_skin_draw) != MOD_OK) {
        coop_log::warn("coop_mod: [SKIN] failed to install Epona draw pre-hook");
    }
    if (mods::hook::add_post<EponaDraw>(finish_epona_skin_draw) != MOD_OK) {
        coop_log::warn("coop_mod: [SKIN] failed to install Epona draw post-hook");
    }
    for (int i = 0; i < kSkinChoiceCount; ++i) {
        ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
        desc.name = kSlotVarName[i];
        desc.type = CONFIG_VAR_STRING;
        desc.default_string = "";
        if (svc_config == nullptr ||
            svc_config->register_var(mod_ctx, &desc, &s_slotVar[i]) != MOD_OK) {
            s_slotVar[i] = 0;
        }
    }
    ConfigVarDesc eponaSkin = CONFIG_VAR_DESC_INIT;
    eponaSkin.name = "epona_skin";
    eponaSkin.type = CONFIG_VAR_STRING;
    eponaSkin.default_string = "";
    if (svc_config == nullptr ||
        svc_config->register_var(mod_ctx, &eponaSkin, &s_eponaSkinVar) != MOD_OK) {
        s_eponaSkinVar = 0;
    }
    ConfigVarDesc cycle = CONFIG_VAR_DESC_INIT;
    cycle.name = "debug_skin_cycle_ticks";
    cycle.type = CONFIG_VAR_INT;
    cycle.default_int = 0;
    if (svc_config == nullptr ||
        svc_config->register_var(mod_ctx, &cycle, &s_cycleVar) != MOD_OK) {
        s_cycleVar = 0;
    }

    ConfigVarDesc outfitCycle = CONFIG_VAR_DESC_INIT;
    outfitCycle.name = "debug_outfit_cycle_ticks";
    outfitCycle.type = CONFIG_VAR_INT;
    outfitCycle.default_int = 0;
    if (svc_config == nullptr ||
        svc_config->register_var(mod_ctx, &outfitCycle, &s_outfitCycleVar) != MOD_OK) {
        s_outfitCycleVar = 0;
    }

    ConfigVarDesc noLocal = CONFIG_VAR_DESC_INIT;
    noLocal.name = "debug_no_local_skin";
    noLocal.type = CONFIG_VAR_BOOL;
    noLocal.default_bool = false;
    if (svc_config == nullptr ||
        svc_config->register_var(mod_ctx, &noLocal, &s_noLocalSkinVar) != MOD_OK) {
        s_noLocalSkinVar = 0;
    }

    ConfigVarDesc files = CONFIG_VAR_DESC_INIT;
    files.name = "model_files";
    files.type = CONFIG_VAR_BOOL;
    files.default_bool = false;
    if (svc_config == nullptr || svc_config->register_var(mod_ctx, &files, &s_filesVar) != MOD_OK) {
        s_filesVar = 0;
    }
    rescan();
    apply_overlays();
    skins_texture_replacements_update();
}

namespace {

std::string shipped_key(std::string rel) {
    for (char& c : rel) {
        c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return rel;
}

int clean_shipped_leftovers(const std::filesystem::path& dir,
                            const std::vector<std::string>& shipped,
                            const std::vector<std::string>& folders) {
    int removed = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        std::error_code tec;
        if (!entry.is_directory(tec)) continue;
        const std::string folder = shipped_key(path_text(entry.path().filename()));
        if (std::find(folders.begin(), folders.end(), folder) == folders.end()) continue;

        std::vector<std::filesystem::path> stale;
        std::vector<std::filesystem::path> subdirs;
        std::error_code wec;
        for (auto it = std::filesystem::recursive_directory_iterator(entry.path(), wec);
             !wec && it != std::filesystem::recursive_directory_iterator(); it.increment(wec)) {
            if (it->is_directory(tec)) {
                subdirs.push_back(it->path());
                continue;
            }
            const std::string name = path_text(it->path().filename());

            if (name.empty() || name[0] == '.') continue;
            std::error_code rec;
            const std::string key =
                shipped_key(std::filesystem::relative(it->path(), dir, rec).generic_string());
            if (rec) continue;
            if (std::find(shipped.begin(), shipped.end(), key) == shipped.end()) {
                stale.push_back(it->path());
            }
        }
        for (const auto& path : stale) {
            std::error_code rec;
            if (std::filesystem::remove(path, rec)) {
                ++removed;
                coop_log::info("coop_mod: [SKIN] removed leftover '{}'", path_text(path));
            }
        }

        std::sort(subdirs.begin(), subdirs.end(), [](const auto& a, const auto& b) {
            return a.native().size() > b.native().size();
        });
        for (const auto& sub : subdirs) {
            std::error_code rec;
            if (std::filesystem::is_empty(sub, rec) && !rec) std::filesystem::remove(sub, rec);
        }
    }
    return removed;
}

}

void unpack_shipped_models() {
    if (svc_resource == nullptr) return;
    std::filesystem::path dir;
    if (!models_dir(dir)) return;

    const std::string stamp = std::string("secondary-wheel ") + kCoopShippedModelsVersion;
    const std::filesystem::path marker = dir / ".shipped";
    {
        std::ifstream have(marker);
        std::string line;
        if (have && std::getline(have, line) && line == stamp) return;
    }

    ResourceBuffer index = RESOURCE_BUFFER_INIT;
    if (svc_resource->load(mod_ctx, "models/index.txt", &index) != MOD_OK || index.data == nullptr) {
        coop_log::info("coop_mod: [SKIN] no models ship with this build");
        return;
    }
    const std::string list(static_cast<const char*>(index.data), index.size);
    svc_resource->free(mod_ctx, &index);

    int written = 0;
    int failed = 0;

    std::vector<std::string> shipped;
    std::vector<std::string> shippedFolders;
    size_t at = 0;
    while (at < list.size()) {
        size_t nl = list.find('\n', at);
        if (nl == std::string::npos) nl = list.size();
        std::string rel = list.substr(at, nl - at);
        at = nl + 1;
        while (!rel.empty() && (rel.back() == '\r' || rel.back() == ' ')) rel.pop_back();
        if (rel.empty() || rel[0] == '#') continue;

        if (rel.find("..") != std::string::npos || rel.find(':') != std::string::npos ||
            rel[0] == '/' || rel[0] == '\\') {
            continue;
        }

        shipped.push_back(shipped_key(rel));
        const std::string& key = shipped.back();
        const size_t slash = key.find('/');
        if (slash != std::string::npos) {
            const std::string folder = key.substr(0, slash);
            if (std::find(shippedFolders.begin(), shippedFolders.end(), folder) ==
                shippedFolders.end()) {
                shippedFolders.push_back(folder);
            }
        }

        ResourceBuffer file = RESOURCE_BUFFER_INIT;
        if (svc_resource->load(mod_ctx, ("models/" + rel).c_str(), &file) != MOD_OK) {
            ++failed;
            continue;
        }
        const std::filesystem::path out = dir / std::filesystem::path(rel);
        std::error_code ec;
        std::filesystem::create_directories(out.parent_path(), ec);
        std::ofstream dst(out, std::ios::binary | std::ios::trunc);
        if (dst && file.data != nullptr && file.size > 0) {
            dst.write(static_cast<const char*>(file.data), static_cast<std::streamsize>(file.size));
        }
        if (dst) {
            ++written;
        } else {
            ++failed;
        }
        svc_resource->free(mod_ctx, &file);
    }

    const int removed = failed == 0 ? clean_shipped_leftovers(dir, shipped, shippedFolders) : 0;

    std::ofstream(marker, std::ios::trunc) << stamp << "\n";
    coop_log::info(
        "coop_mod: [SKIN] unpacked {} shipped files into {} (failed={} removed={})",
        written, path_text(dir), failed, removed);
}

void skins_refresh() {
    rescan();
    forget_failures();
}

int skins_count() {
    if (!s_scanned) rescan();
    return static_cast<int>(s_skins.size());
}

bool skins_hidden(int index) {
    if (!s_scanned) rescan();
    return index >= 0 && index < static_cast<int>(s_skins.size()) && s_skins[index].hidden;
}

bool skins_preset_enabled(const char* name) {
    if (name != nullptr &&
        (std::strcmp(name, "Twilight Wolf") == 0 ||
         std::strcmp(name, "Concept Art Wolf") == 0))
    {
        return false;
    }
    const Skin* skin = find(name);
    return skin == nullptr || skin->presetEnabled;
}

const char* skins_name(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    return s_skins[index].name.c_str();
}

std::string skins_title(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    return s_skins[index].title.empty() ? s_skins[index].name : s_skins[index].title;
}

std::string skins_about_text(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    const Skin& skin = s_skins[index];
    std::string out;
    if (!skin.author.empty()) out += "By " + skin.author + ". ";
    if (!skin.about.empty()) out += skin.about + " ";
    out += "Covers " + skins_parts_text(index) + ".";
    return out;
}

struct EquipName {
    const char* file;
    const char* label;
};

const EquipName kEquipNames[] = {

    {"al_swb.bmd", "Wooden Sword"},
    {"al_swa.bmd", "Ordon Sword"},
    {"al_swm.bmd", "Master Sword"},
    {"al_sha.bmd", "Hylian Shield"},
    {"al_shb.bmd", "Ordon Shield"},
    {"al_shc.bmd", "Wooden Shield"},
};

bool skins_ships_equipment_file(const char* name, const char* file);

void skins_equipment_list(int index, std::vector<std::string>& has,
    std::vector<std::string>& missing) {
    has.clear();
    missing.clear();
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return;
    const std::filesystem::path dir = std::filesystem::u8path(s_skins[index].path) / "equipment";
    std::error_code ec;
    const bool haveDir = is_directory_ci(dir, ec) && !ec;

    auto have_file = [&](const char* file) {
        return skins_ships_equipment_file(s_skins[index].name.c_str(), file);
    };

    auto already = [](const std::vector<std::string>& list, const std::string& label) {
        for (const std::string& item : list) {
            if (item == label) return true;
        }
        return false;
    };
    for (const EquipName& known : kEquipNames) {
        const std::string label = known.label;
        if (already(has, label) || already(missing, label)) continue;
        if (have_file(known.file)) has.push_back(label);
        else missing.push_back(label);
    }

    int others = 0;
    if (haveDir) {
        for (const auto& entry : std::filesystem::directory_iterator(path_ci(dir), ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            const std::string name = path_text(entry.path().filename());
            bool named = false;
            for (const EquipName& known : kEquipNames) {
                if (name == known.file) named = true;
            }
            if (!named) ++others;
        }
    }
    if (others > 0) {
        has.push_back(std::to_string(others) +
                      (others == 1 ? " other item" : " other items"));
    }
}

void skins_outfit_list(int index, std::vector<std::string>& out) {
    out.clear();
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return;
    static const char* const kOutfitName[kSkinOutfitCount] = {
        "Hero's clothes", "Ordon clothes", "Zora armor", "Magic armor", "Wolf",
        "Sumo clothes"};
    for (int o = 0; o < kSkinOutfitCount; ++o) {
        bool anyPart = false;
        for (int i = 0; i < kSkinPartCount; ++i) anyPart = anyPart || s_skins[index].has[o][i];
        if (anyPart) out.push_back(kOutfitName[o]);
    }
}

std::string skins_equipment_text(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    const std::filesystem::path dir = std::filesystem::u8path(s_skins[index].path) / "equipment";
    std::error_code ec;
    if (!is_directory_ci(dir, ec) || ec) return "";
    std::string out;
    int extra = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path_ci(dir), ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        const std::string name = path_text(entry.path().filename());
        const char* label = nullptr;
        for (const EquipName& known : kEquipNames) {
            if (name == known.file) label = known.label;
        }
        if (label == nullptr) {
            ++extra;
            continue;
        }
        if (!out.empty()) out += ", ";
        out += label;
    }
    if (out.empty() && extra == 0) return "";
    if (extra > 0) {
        if (!out.empty()) out += ", ";
        out += "and " + std::to_string(extra) + " more";
    }
    return out;
}

std::string skins_author_text(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    const std::string& author = s_skins[index].author;
    return author.empty() ? std::string() : "By " + author;
}

std::string skins_own_words(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    return s_skins[index].about;
}

std::string skins_parts_text(int index) {
    if (!s_scanned) rescan();
    if (index < 0 || index >= static_cast<int>(s_skins.size())) return "";
    static const char* const kOutfitName[kSkinOutfitCount] = {
        "hero's clothes", "Ordon", "Zora", "magic armor", "wolf", "sumo clothes"};
    std::string out;
    for (int o = 0; o < kSkinOutfitCount; ++o) {
        bool anyPart = false;
        for (int i = 0; i < kSkinPartCount; ++i) anyPart = anyPart || s_skins[index].has[o][i];
        if (!anyPart) continue;
        if (!out.empty()) out += ", ";
        out += kOutfitName[o];
    }
    return out.empty() ? "nothing usable" : out;
}

bool skins_can_open_folder() {
    return kCanOpenFolder;
}

void skins_open_folder() {
    if (!kCanOpenFolder) return;
#if COOP_HAS_FILE_MANAGER
    const std::string dir = skins_folder_path();
    if (dir.empty()) return;
#if defined(_WIN32)

    const std::wstring wide = L"explorer \"" + std::filesystem::u8path(dir).wstring() + L"\"";
    _wsystem(wide.c_str());
#elif defined(__APPLE__)
    const std::string command = "open \"" + dir + "\"";
    std::system(command.c_str());
#else
    const std::string command = "xdg-open \"" + dir + "\" &";
    std::system(command.c_str());
#endif
#endif
}

std::string skins_folder_path() {
    std::filesystem::path dir;
    if (!models_dir(dir)) return "";
    return path_text(dir);
}

ConfigVarHandle skins_files_var() {
    return s_filesVar;
}

void skins_files_changed() {
    apply_overlays();
}

const char* skins_slot_label(int slot) {
    if (slot < 0 || slot >= kSkinChoiceCount) return "";
    return kSlotLabel[slot];
}

std::string skins_local_slot(int slot) {
    if (slot < 0 || slot >= kSkinChoiceCount) return "";
    return cfg_string(s_slotVar[slot], "");
}

std::string skins_local_epona_skin() {
    return cfg_string(s_eponaSkinVar, "");
}

void skins_set_local_epona_skin(const char* name) {
    if (s_eponaSkinVar == 0 || svc_config == nullptr) return;
    const char* value = name != nullptr ? name : "";
    if (std::strcmp(value, kOotStyleEponaName) != 0 &&
        std::strcmp(value, kTwilightEponaName) != 0 &&
        std::strcmp(value, kNightwalkerEponaName) != 0)
    {
        value = "";
    }
    svc_config->set_string(mod_ctx, s_eponaSkinVar, value);
    skins_texture_replacements_update();
    send_skin_choices();
}

void skins_local_choices(SkinChoices* out) {
    if (out == nullptr) return;
    std::memset(out, 0, sizeof(*out));
    for (int i = 0; i < kSkinChoiceCount; ++i) {

        if (i == kSkinChoiceVoice && skins_local_slot(i) == kSilentVoice) {
            std::strncpy(out->name[i], kSilentVoice, kSkinNameMax - 1);
            continue;
        }
        const Skin* skin = find(skins_local_slot(i).c_str());
        if (skin == nullptr) continue;
        std::strncpy(out->name[i], skin->name.c_str(), kSkinNameMax - 1);
        out->hash[i] = skin->hash;
    }
}

std::string preset_value(const char* name, int slot) {
    const std::string value = name != nullptr ? name : "";
    if (slot != kSkinChoiceVoice || value.empty()) return value;
    const Skin* skin = find(name);
    return skin != nullptr && skin->silentVoice ? std::string(kSilentVoice) : value;
}

bool skins_all_same(const char* name) {
    const bool heroOfTime = name != nullptr && std::strcmp(name, "Hero of Time") == 0;
    const std::string eponaSkin = skins_local_epona_skin();
    if (heroOfTime ? eponaSkin != kOotStyleEponaName : !eponaSkin.empty()) return false;
    for (int i = 0; i < kSkinChoiceCount; ++i) {

        if (i >= kSkinChoiceFirstItem) {
            if (!skins_local_slot(i).empty()) return false;
            continue;
        }
        if (skins_local_slot(i) != preset_value(name, i)) return false;
    }
    return true;
}

void after_choice_change() {
    forget_failures();
    apply_overlays();
    local_skin_rebuild_link();
    colors_invalidate_self();
    skins_texture_replacements_update();

    send_skin_choices();
}

void skins_set_local_slot(int slot, const char* name) {
    if (slot < 0 || slot >= kSkinChoiceCount || s_slotVar[slot] == 0 || svc_config == nullptr) {
        return;
    }
    svc_config->set_string(mod_ctx, s_slotVar[slot], name != nullptr ? name : "");
    after_choice_change();
}

void skins_set_local_all(const char* name) {
    if (svc_config == nullptr) return;
    const bool heroOfTime = name != nullptr && std::strcmp(name, "Hero of Time") == 0;
    for (int i = 0; i < kSkinChoiceCount; ++i) {
        if (s_slotVar[i] == 0) continue;

        const bool piece = i >= kSkinChoiceFirstItem;
        const std::string value = preset_value(name, i);
        svc_config->set_string(mod_ctx, s_slotVar[i], (piece || name == nullptr) ? "" : value.c_str());
    }
    if (s_eponaSkinVar != 0) {
        svc_config->set_string(
            mod_ctx, s_eponaSkinVar, heroOfTime ? kOotStyleEponaName : "");
    }
    after_choice_change();
}

int skins_slot_for_outfit(int outfit) {
    switch (outfit) {
    case kSkinOutfitSumo: return kSkinChoiceSumo;
    case kSkinOutfitOrdon: return kSkinChoiceOrdon;
    case kSkinOutfitZora: return kSkinChoiceZora;
    case kSkinOutfitMagic: return kSkinChoiceMagic;
    case kSkinOutfitWolf: return kSkinChoiceWolf;
    default: return kSkinChoiceHero;
    }
}

bool skins_have(const char* name, uint32_t hash) {
    const Skin* skin = find(name);
    return skin != nullptr && skin->hash == hash;
}

J3DModelData* part_data(const char* name, int outfit, int part, bool forLink);

J3DModelData* skins_part_data(const char* name, int outfit, int part) {
    return part_data(name, outfit, part, false);
}

J3DModelData* part_data(const char* name, int outfit, int part, bool forLink) {
    if (part < 0 || part >= kSkinPartCount) return nullptr;
    if (outfit < 0 || outfit >= kSkinOutfitCount) return nullptr;
    const Skin* skin = find(name);

    if (skin == nullptr || skin->builtin || !skin->has[outfit][part]) return nullptr;
    if (skin->baseWolf && outfit == kSkinOutfitWolf && part == kSkinPartBody) return nullptr;

    for (const LoadedPart& loaded : s_loadedParts) {
        if (loaded.part != part || loaded.outfit != outfit || loaded.skin != skin->name ||
            loaded.forLink != forLink) {
            continue;
        }
        if (loaded.failed) return nullptr;
        return loaded.data;
    }

    const std::filesystem::path dir = kOutfitDir[outfit][0] == '\0'
                                          ? std::filesystem::u8path(skin->path)
                                          : std::filesystem::u8path(skin->path) / kOutfitDir[outfit];
    const std::string file = path_text((dir / kPartFile[part]));
    LoadedPart loaded;
    loaded.skin = skin->name;
    loaded.outfit = outfit;
    loaded.part = part;
    loaded.forLink = forLink;

    loaded.data = forLink ? loadBmdDataForLink(file.c_str()) : loadBmdDataForPuppet(file.c_str());
    if (loaded.data == nullptr) loaded.data = loadBmdDataFromFile(file.c_str());
    loaded.failed = loaded.data == nullptr;
    s_loadedParts.push_back(loaded);
    if (loaded.failed) {
        coop_log::warn("coop_mod: [SKIN] '{}' {} failed to load, using game part",
            skin->name, kPartFile[part]);
        return nullptr;
    }
    return loaded.data;
}

J3DModel* skins_part_model(const char* name, int outfit, int part, float scale) {
    J3DModelData* data = skins_part_data(name, outfit, part);
    return data != nullptr ? modelFromData(data, cXyz(scale, scale, scale)) : nullptr;
}

J3DModelData* skins_local_cutscene_data(const char* file) {
    const std::string mine = skins_local_slot(skins_slot_for_outfit(local_skin_outfit()));
    if (mine.empty() || file == nullptr || file[0] == '\0') return nullptr;
    const Skin* skin = find(mine.c_str());
    if (skin == nullptr || skin->builtin) return nullptr;

    for (const LoadedCutscene& loaded : s_loadedCutscenes) {
        if (loaded.skin != skin->name || loaded.file != file) continue;
        return loaded.failed ? nullptr : loaded.data;
    }
    const std::filesystem::path path =
        std::filesystem::u8path(skin->path) / "cutscene" / file;
    std::error_code ec;
    LoadedCutscene loaded;
    loaded.skin = skin->name;
    loaded.file = file;
    if (exists_ci(path, ec) && !ec) {
        loaded.data = loadBmdDataFromFile(path_text(path).c_str());
    }
    loaded.failed = loaded.data == nullptr;
    s_loadedCutscenes.push_back(loaded);
    return loaded.data;
}

bool skins_local_disabled() {
    return cfg_bool(s_noLocalSkinVar, false);
}

J3DModelData* skins_local_part_data(int outfit, int part) {
    if (skins_local_disabled()) return nullptr;
    const std::string mine = skins_local_slot(skins_slot_for_outfit(outfit));
    if (mine.empty()) return nullptr;
    return part_data(mine.c_str(), outfit, part, true);
}

J3DModelData* equipment_data(const char* name, const char* file, bool forLink);

J3DModelData* skins_equipment_data(const char* name, const char* file) {
    return equipment_data(name, file, false);
}

J3DModelData* equipment_data(const char* name, const char* file, bool forLink) {
    if (file == nullptr || file[0] == '\0') return nullptr;
    const Skin* skin = find(name);
    if (skin == nullptr) return nullptr;
    if (skin->builtin) return skin->name == kGanondorfSkin ? ganon_skin_equipment(file) : nullptr;
    const std::string key =
        std::string(skin->name) + (forLink ? "/equipment-link/" : "/equipment/") + file;
    for (const LoadedCutscene& loaded : s_loadedCutscenes) {
        if (loaded.skin != key) continue;
        return loaded.failed ? nullptr : loaded.data;
    }
    std::filesystem::path path = std::filesystem::u8path(skin->path) / "equipment" / file;
    std::error_code ec;
    LoadedCutscene loaded;
    loaded.skin = key;
    if (exists_ci(path, ec) && !ec) {
        loaded.data = forLink ? loadBmdDataForLink(path_text(path).c_str())
                              : loadBmdDataForPuppet(path_text(path).c_str());
        if (loaded.data == nullptr) loaded.data = loadBmdDataFromFile(path_text(path).c_str());
    }
    loaded.failed = loaded.data == nullptr;
    s_loadedCutscenes.push_back(loaded);
    return loaded.data;
}

bool skins_ships_equipment_file(const char* name, const char* file) {
    if (!s_scanned) rescan();
    const Skin* skin = find(name);
    if (skin == nullptr || file == nullptr) return false;
    if (skin->builtin) {
        return (skin->name == kGanondorfSkin && ganon_skin_ships(file)) ||
               (skin->name == kHerosShadeSkin && shade_skin_ships(file));
    }
    std::error_code ec;
    const std::filesystem::path root(skin->path);
    if (exists_ci(root / "equipment" / file, ec) && !ec) return true;
    for (int o = 0; o < kSkinOutfitCount; ++o) {
        const std::filesystem::path dir =
            (kOutfitDir[o][0] == '\0') ? root : root / kOutfitDir[o];
        if (exists_ci(dir / file, ec) && !ec) return true;
    }
    return false;
}

struct AramFile {
    uint16_t index;
    const char* file;
    int slot;
};
const AramFile kAramFiles[] = {
    {0x310, "al_bottle.bmd", kSkinChoiceBottle},
    {0x31C, "o_gd_hk.bmd", kSkinChoiceBottle},
    {0x31D, "o_gd_nv.bmd", kSkinChoiceBottle},
    {0x31E, "o_gd_worm.bmd", kSkinChoiceBottle},
    {0x311, "al_ib.bmd", kSkinChoiceBallAndChain},
    {0x314, "al_bow.bmd", kSkinChoiceBow},
    {0x315, "al_crod.bmd", kSkinChoiceFishingRod},
    {0x316, "al_hs.bmd", kSkinChoiceHookshot},
    {0x317, "al_hs_kusari.bmd", kSkinChoiceHookshot},
    {0x318, "al_hs_tip.bmd", kSkinChoiceHookshot},
    {0x319, "al_pachi.bmd", kSkinChoiceSlingshot},
};

const char* skins_aram_file_for_index(uint16_t index) {
    for (const AramFile& entry : kAramFiles) {
        if (entry.index == index) return entry.file;
    }
    return nullptr;
}

J3DModelData* skins_local_aram_data(uint16_t index) {
    for (const AramFile& entry : kAramFiles) {
        if (entry.index != index) continue;
        std::string mine = skins_local_slot(entry.slot);
        if (mine.empty()) mine = skins_local_slot(kSkinChoiceEquipment);
        if (mine.empty()) return nullptr;
        return skins_equipment_data(mine.c_str(), entry.file);
    }
    return nullptr;
}

int skins_slot_for_equipment_file(const char* file) {
    if (file == nullptr) return kSkinChoiceEquipment;
    for (const EquipSlotFile& entry : kEquipSlotFiles) {
        if (std::strcmp(entry.file, file) == 0) return entry.slot;
    }

    for (const AramFile& entry : kAramFiles) {
        if (std::strcmp(entry.file, file) == 0) return entry.slot;
    }
    return kSkinChoiceEquipment;
}

J3DModelData* skins_local_equipment_data(const char* file) {
    if (file == nullptr || file[0] == '\0') return nullptr;

    if (std::strcmp(file, "footmark.bmd") == 0) {
        const std::string worn = skins_local_slot(skins_slot_for_outfit(local_skin_outfit()));
        return worn.empty() ? nullptr : equipment_data(worn.c_str(), file, true);
    }

    const int slot = skins_slot_for_equipment_file(file);
    std::string mine = skins_local_slot(slot);
    if (mine.empty() && slot != kSkinChoiceEquipment) {
        mine = skins_local_slot(kSkinChoiceEquipment);
    }
    if (mine.empty()) return nullptr;
    return equipment_data(mine.c_str(), file, true);
}

bool skins_covers_outfit(const char* name, int outfit) {
    if (name == nullptr || name[0] == '\0') return true;
    if (outfit < 0 || outfit >= kSkinOutfitCount) return true;
    const Skin* skin = find(name);
    if (skin == nullptr) return true;
    for (int part = 0; part < kSkinPartCount; ++part) {
        if (skin->has[outfit][part]) return true;
    }
    return false;
}

bool skins_covers_slot(const char* name, int slot) {
    if (name == nullptr || name[0] == '\0') return true;
    const Skin* skin = find(name);
    if (skin == nullptr) return true;
    switch (slot) {
    case kSkinChoiceEquipment: return skin->hasEquipment;
    case kSkinChoiceCutscenes: return skin->hasCutscenes;
    case kSkinChoiceVoice: return skin->hasVoice;
    default: break;
    }

    if (slot >= kSkinChoiceFirstItem) {
        for (const EquipSlotFile& entry : kEquipSlotFiles) {
            if (entry.slot != slot) continue;
            if (skins_ships_equipment_file(name, entry.file)) return true;
        }

        for (const AramFile& entry : kAramFiles) {
            if (entry.slot != slot) continue;
            if (skins_ships_equipment_file(name, entry.file)) return true;
        }
        return false;
    }
    for (int outfit = 0; outfit < kSkinOutfitCount; ++outfit) {
        if (skins_slot_for_outfit(outfit) == slot) return skins_covers_outfit(name, outfit);
    }
    return true;
}

std::string slot_missing_text(int slot) {
    if (slot == kSkinChoiceWolf) return "This model does not have a wolf.";
    std::string what = skins_slot_label(slot);
    if (!what.empty()) what[0] = static_cast<char>(std::tolower(what[0]));
    return "This model does not have " + what + ".";
}

void skins_warn_update(int outfit) {

    static int s_lastOutfit = -1;
    static std::string s_lastName;
    if (outfit < 0 || outfit >= kSkinOutfitCount) return;
    const std::string name = skins_local_slot(skins_slot_for_outfit(outfit));
    if (outfit == s_lastOutfit && name == s_lastName) return;
    s_lastOutfit = outfit;
    s_lastName = name;

    if (name.empty() || skins_covers_outfit(name.c_str(), outfit)) return;

    const Skin* skin = find(name.c_str());
    const std::string title = skin != nullptr && !skin->title.empty() ? skin->title : name;
    coop_toast(title.c_str(), slot_missing_text(skins_slot_for_outfit(outfit)).c_str());
}
