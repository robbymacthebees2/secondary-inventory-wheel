#pragma once

#include "mods/service.hpp"

#define TWILIT_ESSENTIALS_STAMINA_STATE_V1_0_SIZE 24u

#define TWILIT_ESSENTIALS_STAMINA_SERVICE_ID "com.dusklight.twilit_essentials.stamina"
#define TWILIT_ESSENTIALS_STAMINA_SERVICE_MAJOR 1u
#define TWILIT_ESSENTIALS_STAMINA_SERVICE_MINOR 3u

struct TwilitEssentialsStaminaState {
    uint32_t struct_size;
    uint32_t enabled;
    uint32_t gameplay;
    uint32_t exhausted;
    float current;
    float maximum;
    float recover_at;
    float regen_delay;
};

#define TWILIT_ESSENTIALS_STAMINA_STATE_INIT \
    {sizeof(TwilitEssentialsStaminaState), 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f}

enum TwilitEssentialsStaminaSource : uint32_t {
    TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT = 13,
    TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT = 16,
};

struct TwilitEssentialsStaminaSourceInfo {
    uint32_t struct_size;
    uint32_t enabled;
    float cost_multiplier;
};

#define TWILIT_ESSENTIALS_STAMINA_SOURCE_INFO_INIT \
    {sizeof(TwilitEssentialsStaminaSourceInfo), 0, 0.0f}

struct TwilitEssentialsStaminaService {
    ServiceHeader header;
    ModResult (*get_state)(ModContext* caller, TwilitEssentialsStaminaState* out_state);
    ModResult (*try_consume)(ModContext* caller, float amount);
    ModResult (*drain)(ModContext* caller, float amount);
    ModResult (*restore)(ModContext* caller, float amount);
    ModResult (*deny)(ModContext* caller);
    ModResult (*get_source)(ModContext* caller, uint32_t source,
        TwilitEssentialsStaminaSourceInfo* out_info);
};

MOD_DECLARE_SERVICE(TwilitEssentialsStaminaService, svc_te_stamina,
    TWILIT_ESSENTIALS_STAMINA_SERVICE_ID, TWILIT_ESSENTIALS_STAMINA_SERVICE_MAJOR,
    TWILIT_ESSENTIALS_STAMINA_SERVICE_MINOR);
