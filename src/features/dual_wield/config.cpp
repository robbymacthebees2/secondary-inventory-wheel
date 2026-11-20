#include "config.hpp"

#include "mods/svc/config.h"
#include "mods/svc/log.h"

namespace dawnlight {
namespace {
bool s_enabledReadFailed = false;
bool s_swordReadFailed = false;
}

bool dual_wield_enabled() {
    bool enabled = false;
    if (svc_config == nullptr ||
        svc_config->get_bool(mod_ctx, g_dualWieldEnabled, &enabled) != MOD_OK)
    {
        if (!s_enabledReadFailed && svc_log != nullptr) {
            svc_log->error(mod_ctx, "Dual Wield: could not read enabled setting");
            s_enabledReadFailed = true;
        }
        return false;
    }
    return enabled;
}

ConfigVarHandle dual_wield_config_var() {
    return g_dualWieldEnabled;
}

SecondSword second_sword() {
    int64_t selected = static_cast<int64_t>(SecondSword::Ordon);
    if (svc_config == nullptr ||
        svc_config->get_int(mod_ctx, g_dualWieldSword, &selected) != MOD_OK)
    {
        if (!s_swordReadFailed && svc_log != nullptr) {
            svc_log->error(mod_ctx, "Dual Wield: could not read selected sword setting");
            s_swordReadFailed = true;
        }
        return SecondSword::Ordon;
    }
    if (selected < static_cast<int64_t>(SecondSword::Wooden) ||
        selected > static_cast<int64_t>(SecondSword::Master))
    {
        if (svc_log != nullptr) {
            svc_log->warn(mod_ctx, "Dual Wield: selected sword setting is outside the supported range");
        }
        return SecondSword::Ordon;
    }
    return static_cast<SecondSword>(selected);
}

bool glide_active_for(daAlink_c* link) {
    (void)link;
    return g_manualCuccoGlideActive;
}
}
