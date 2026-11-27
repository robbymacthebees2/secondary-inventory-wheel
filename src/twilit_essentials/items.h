#pragma once

#include "mods/service.hpp"

#define TWILIT_ESSENTIALS_ITEMS_SERVICE_ID "com.dusklight.twilit_essentials.items"
#define TWILIT_ESSENTIALS_ITEMS_SERVICE_MAJOR 1u
#define TWILIT_ESSENTIALS_ITEMS_SERVICE_MINOR 1u

// Maximum length of an item id.
#define TWILIT_ESSENTIALS_ITEM_ID_MAX 63u

// Items per Modded page; additional pages appear automatically.
#define TWILIT_ESSENTIALS_ITEMS_PER_PAGE 20u
#define TWILIT_ESSENTIALS_MAX_ITEMS TWILIT_ESSENTIALS_ITEMS_PER_PAGE

#define TWILIT_ESSENTIALS_ITEM_BUTTON_NONE 0xFFu

typedef uint64_t TwilitEssentialsItemHandle;

struct TwilitEssentialsItemDesc {
    uint32_t struct_size;
    const char* id;
    const char* name;
    const char* description;
    const char* icon;
    uint32_t icon_item;
    uint32_t (*is_unlocked)(void* user_data);
    uint32_t (*use)(void* user_data);
    uint32_t (*get_count)(void* user_data, int32_t* out_count, int32_t* out_max);
    void* user_data;
    float icon_scale;
    const char* model;
    const char* model_btk;
    const char* model_brk;
    float model_scale;
};

#define TWILIT_ESSENTIALS_ITEM_DESC_INIT \
    {sizeof(TwilitEssentialsItemDesc), nullptr, nullptr, nullptr, nullptr, 0xFFu, nullptr, nullptr, \
        nullptr, nullptr, 1.0f, nullptr, nullptr, nullptr, 1.0f}

struct TwilitEssentialsItemsService {
    ServiceHeader header;
    ModResult (*add_item)(
        ModContext* caller, const TwilitEssentialsItemDesc* desc,
        TwilitEssentialsItemHandle* out_handle);
    ModResult (*remove_item)(ModContext* caller, TwilitEssentialsItemHandle handle);
    ModResult (*get_item_button)(
        ModContext* caller, TwilitEssentialsItemHandle handle, uint32_t* out_button);
};

MOD_DECLARE_SERVICE(
    TwilitEssentialsItemsService, svc_te_items, TWILIT_ESSENTIALS_ITEMS_SERVICE_ID,
    TWILIT_ESSENTIALS_ITEMS_SERVICE_MAJOR, TWILIT_ESSENTIALS_ITEMS_SERVICE_MINOR);
