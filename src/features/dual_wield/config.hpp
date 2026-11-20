#pragma once

#include "dual_wield_sword.hpp"
#include "mods/api.h"

namespace dawnlight {
bool dual_wield_enabled();
ConfigVarHandle dual_wield_config_var();
SecondSword second_sword();
bool glide_active_for(daAlink_c* link);
}
