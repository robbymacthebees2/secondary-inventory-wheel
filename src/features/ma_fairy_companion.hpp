#pragma once

#include <cstdint>

#include "mods/svc/actor.h"
#include "f_op/f_op_actor.h"
#include "SSystem/SComponent/c_phase.h"
#include "Z2AudioLib/Z2Creature.h"

#define MFAIRYC_NAME "SIWFair"

// Color identifier (indices into fairy tint table)
enum FairyColor_e {
    FAIRY_COLOR_BLUE = 0,  // Navi
    FAIRY_COLOR_ORANGE = 1,  // Tatl
    FAIRY_COLOR_PURPLE = 2,  // Tael
    FAIRY_COLOR_GREEN = 3,  // Extra
};

enum FairyAppearance_e {
    FAIRY_APPEARANCE_FAIRY = 0,
    FAIRY_APPEARANCE_MIDNA = 1,
    FAIRY_APPEARANCE_MIDNA_REGULAR = 2,
};

class maFairyCompanion_c : public fopAc_ac_c {
public:
    // --- Model & Audio ---
    mDoExt_McaMorfSO* mpModelMorf;
    Z2Creature        mSound;

    // --- Flight & Follow Dynamics ---
    f32 mOrbitAngle;    // Orbit angle around the lock-on target (advances only while locked on)
    f32 mSwayPhase;     // Lateral sway phase behind Link
    f32 mBobPhase;      // Vertical bob phase (also drives the aura pulse)

    // --- Particle Systems ---
    u32 mParticleId;    // Soft trail emitter ID (0x72F)
    u32 mSparkleId;     // Sparkle emitter ID (0x731)
    u32 mAuraId;        // Body aura emitter ID (0x730)
    u8  mParticleTimer; // Trail timer
    u8  mSparkleTimer;  // Sparkle timer
    u8  mAuraTimer;     // Aura timer
    u8  mColorId;       // Active color index
    u8  mAppearanceId;

    virtual ~maFairyCompanion_c();

    cPhs_Step create();
    int       CreateHeap();
    int       Delete();
    int       Execute();
    int       Draw();
    void      setBaseMtx();

    // Material tinting (applies color to model TEV registers after environment pass)
    static void applyFairyTint(J3DModel* i_model, u8 colorId);

    // --- Global Configuration (Shared across respawns) ---
    static u8   sSelectedColor;
    static void setSelectedColor(u8 colorId);
    static u8   getSelectedColor() { return sSelectedColor; }

    static u8   sSelectedAppearance;
    static void setSelectedAppearance(u8 appearanceId);
    static u8   getSelectedAppearance() { return sSelectedAppearance; }

    static bool sCalled;
    static void setCalled(bool called) { sCalled = called; }
    static bool isCalled() { return sCalled; }

    static bool sLightEnabled;
    static void setLightEnabled(bool enabled) { sLightEnabled = enabled; }
    static bool getLightEnabled() { return sLightEnabled; }

    // --- Actor Profile & Registration ---
    static int createHeapCallBack(fopAc_ac_c*);

    static s16                    sProcName;
    static ActorHandle            sActorHandle;
    static const ActorProfileDesc sProfile;

    static u32  sExecTick;
};

bool fairy_companion_midna_is_out();
bool fairy_companion_midna_is_regular();