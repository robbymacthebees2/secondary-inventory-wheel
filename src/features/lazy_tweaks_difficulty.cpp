namespace lazy_tweaks_difficulty {
namespace {

DEFINE_HOOK(&daAlink_c::damageMagnification, AlinkDamageMagnification);
DEFINE_HOOK(&daAlink_c::procCoDeadInit, AlinkDeadInit);
DEFINE_HOOK(&daItem_c::_daItem_create, ItemCreate);
DEFINE_HOOK(&dScnPly_c::calcPauseTimer, CalcPauseTimer);

template <typename T, typename = void>
struct HasStageOriginFlag : std::false_type {};

template <typename T>
struct HasStageOriginFlag<T, std::void_t<decltype(std::declval<T>().mFromStageData)>>
    : std::true_type {};

bool s_configReadFailureReported = false;
bool s_rupeeOriginWarningReported = false;

bool read_bool(ConfigVarHandle setting, bool fallback) {
    bool value = fallback;
    if (setting == 0 ||
        svc_config->get_bool(mod_ctx, setting, &value) != MOD_OK)
    {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read Lazy Tweaks difficulty settings");
            s_configReadFailureReported = true;
        }
        return fallback;
    }

    s_configReadFailureReported = false;
    return value;
}

int64_t read_int(ConfigVarHandle setting, int64_t fallback, int64_t minimum, int64_t maximum) {
    int64_t value = fallback;
    if (setting == 0 || svc_config->get_int(mod_ctx, setting, &value) != MOD_OK) {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read Lazy Tweaks difficulty settings");
            s_configReadFailureReported = true;
        }
        return fallback;
    }

    s_configReadFailureReported = false;
    return std::clamp(value, minimum, maximum);
}

bool is_rupee_item(u8 itemNo) {
    return itemNo >= dItemNo_GREEN_RUPEE_e && itemNo <= dItemNo_SILVER_RUPEE_e;
}

template <typename T>
bool is_stage_placed(const T* append) {
    if constexpr (HasStageOriginFlag<T>::value) {
        return append != nullptr && append->mFromStageData;
    }
    return false;
}

}

bool rupee_drop_filter_supported() {
    return HasStageOriginFlag<fopAcM_prm_class>::value;
}

void apply_received_damage_settings(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || retval == nullptr) {
        return;
    }

    const BOOL checkZoraMultiplier = mods::arg<BOOL>(args, 1);
    const int damageMode = mods::arg<int>(args, 2);
    const bool wolfDamage =
        link->checkWolf() && !link->checkCargoCarry() && damageMode == 0;
    const bool zoraDamage =
        link->checkZoraWearAbility() && checkZoraMultiplier;
    f32 baseMultiplier = *static_cast<f32*>(retval);
    if (wolfDamage) {
        baseMultiplier /= 2.0f;
    } else if (zoraDamage) {
        baseMultiplier /= 10.0f;
    }

    if (read_bool(g_lazyTweaksInstantDeath, false)) {
        *static_cast<f32*>(retval) = 9999.0f;
        return;
    }

    baseMultiplier *= static_cast<f32>(read_int(
        g_lazyTweaksReceivedDamageMultiplier,
        kLazyTweaksReceivedDamageDefaultMultiplier,
        kLazyTweaksReceivedDamageMinMultiplier,
        kLazyTweaksReceivedDamageMaxMultiplier));

    if (wolfDamage &&
        !read_bool(g_lazyTweaksSturdierWolfLink, false))
    {
        baseMultiplier *= 2.0f;
    } else if (zoraDamage &&
               !read_bool(g_lazyTweaksInsulatedZoraArmor, false))
    {
        baseMultiplier *= 10.0f;
    } else if (read_bool(g_lazyTweaksProgressiveDefense, false) &&
               dComIfGs_isEventBit(0xD04))
    {
        baseMultiplier += 2.0f;
    }

    *static_cast<f32*>(retval) = baseMultiplier;
}

HookAction apply_death_rupee_tax(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || link->mProcID == daAlink_c::PROC_DEAD) {
        return HOOK_CONTINUE;
    }

    const bool halfTax = read_bool(g_lazyTweaksHalfDeathTax, false);
    const bool quarterTax = read_bool(g_lazyTweaksQuarterDeathTax, false);
    const int64_t flatTax = read_int(
        g_lazyTweaksDeathRupeeTax, kLazyTweaksDeathRupeeTaxDefault, 0,
        kLazyTweaksDeathRupeeTaxMax);
    if (!halfTax && !quarterTax && flatTax == 0) {
        return HOOK_CONTINUE;
    }

    const u16 rupees = dComIfGs_getRupee();
    u16 remaining = rupees;
    if (halfTax) {
        remaining = rupees / 2;
    }
    if (quarterTax) {
        remaining = static_cast<u16>(rupees * 3 / 4);
    }
    if (flatTax != 0) {
        remaining = static_cast<u16>(
            rupees > flatTax ? rupees - flatTax : 0);
    }
    dComIfGs_setRupee(remaining);
    return HOOK_CONTINUE;
}

HookAction suppress_configured_item_drops(
    ModContext*, void* args, void* retval, void*)
{
    auto* item = mods::arg<daItem_c*>(args, 0);
    if (item == nullptr || retval == nullptr) {
        return HOOK_CONTINUE;
    }

    const u8 itemNo = daItem_prm::getItemNo(item);
    bool suppress = false;
    if (isHeart(itemNo)) {
        suppress = read_bool(g_lazyTweaksNoHeartDrops, false);
        const int64_t rate = read_int(
            g_lazyTweaksHeartDropRate, kLazyTweaksHeartDropRateDefaultPercent,
            kLazyTweaksHeartDropRateMinPercent, kLazyTweaksHeartDropRateMaxPercent);
        if (!suppress && rate < 100) {
            suppress = cM_rndF(1.0f) > static_cast<f32>(rate) / 100.0f;
        }
    }
    if (!suppress && isArrow(itemNo)) {
        suppress = read_bool(g_lazyTweaksNoArrowDrops, false);
    }
    if (!suppress && is_rupee_item(itemNo) &&
        read_bool(g_lazyTweaksNoRupeeDrops, false))
    {
        if (rupee_drop_filter_supported()) {
            suppress = !is_stage_placed(fopAcM_GetAppend(item));
        } else if (!s_rupeeOriginWarningReported) {
            svc_log->warn(
                mod_ctx,
                "No Rupee Drops is unavailable: the runtime does not expose stage-origin metadata");
            s_rupeeOriginWarningReported = true;
        }
    }

    if (!suppress) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = cPhs_ERROR_e;
    return HOOK_SKIP_ORIGINAL;
}

void disable_hitstop(ModContext*, void*, void* retval, void*) {
    if (!read_bool(g_lazyTweaksNoHitstop, false) || retval == nullptr) {
        return;
    }

    dScnPly_c::pauseTimer = 0;
    dScnPly_c::nextPauseTimer = 0;
    *static_cast<s8*>(retval) = 0;
}

}
