static Mtx g_armosHammerIdentityMatrix = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
};
static Mtx g_armosHammerHandMatrix = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
};
static bool g_armosHammerHandMatrixValid = false;
static bool g_armosHammerInterpolationErrorLogged = false;

constexpr u16 kArmosHammerMaxSwordShapes = 64;

struct ArmosHammerSwordVisibility {
    J3DModel* model = nullptr;
    u16 shapeCount = 0;
    std::array<bool, kArmosHammerMaxSwordShapes> wasHidden{};
};

static ArmosHammerSwordVisibility g_armosHammerSwordVisibility{};
static const GXColor g_armosHammerDarkSteel{52, 61, 70, 255};
static const GXColor g_armosHammerStone{151, 145, 119, 255};
static const GXColor g_armosHammerBrass{190, 133, 55, 255};
static const GXColor g_armosHammerRune{38, 190, 183, 255};

class ArmosHammerCylinderPacket : public mDoExt_cylinderMPacket {
public:
    ArmosHammerCylinderPacket(Mtx matrix, const GXColor& color)
        : mDoExt_cylinderMPacket(matrix, color, TRUE)
    {}

    Mtx mLocalMatrix{};

    void draw() override {
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL,
                      GX_DF_CLAMP, GX_AF_NONE);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevColor(GX_TEVREG0, mColor);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                        GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                        GX_TRUE, GX_TEVPREV);

        if (mClipZ) {
            GXSetZMode(GX_ENABLE, GX_LEQUAL, GX_ENABLE);
        } else {
            GXSetZMode(GX_DISABLE, GX_LEQUAL, GX_DISABLE);
        }

        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
        GXSetCullMode(GX_CULL_BACK);
        GXSetClipMode(GX_CLIP_ENABLE);

#if TARGET_PC
        Mtx handMatrix;
        if (!svc_interp->lookup_replacement_mtx(&g_armosHammerHandMatrix, handMatrix)) {
            cMtx_copy(g_armosHammerHandMatrix, handMatrix);
        }
        Mtx worldMatrix;
        Mtx modelViewMtx;
        cMtx_concat(handMatrix, mLocalMatrix, worldMatrix);
        cMtx_concat(j3dSys.getViewMtx(), worldMatrix, modelViewMtx);
        GXLoadPosMtxImm(modelViewMtx, 0);

        Mtx normalMtx;
        cMtx_inverseTranspose(modelViewMtx, normalMtx);
        GXLoadNrmMtxImm(normalMtx, 0);
#else
        Mtx worldMatrix;
        cMtx_concat(g_armosHammerHandMatrix, mLocalMatrix, worldMatrix);
        cMtx_concat(j3dSys.getViewMtx(), worldMatrix, mMatrix);
        GXLoadPosMtxImm(mMatrix, 0);
        cMtx_inverseTranspose(mMatrix, mMatrix);
        GXLoadNrmMtxImm(mMatrix, 0);
#endif
        GXSetCurrentMtx(0);
        GXDrawCylinder(8);
    }
};

static ArmosHammerCylinderPacket g_armosHammerPackets[] = {
    {g_armosHammerIdentityMatrix, g_armosHammerDarkSteel},
    {g_armosHammerIdentityMatrix, g_armosHammerBrass},
    {g_armosHammerIdentityMatrix, g_armosHammerBrass},
    {g_armosHammerIdentityMatrix, g_armosHammerStone},
    {g_armosHammerIdentityMatrix, g_armosHammerBrass},
    {g_armosHammerIdentityMatrix, g_armosHammerRune},
};

void record_armos_hammer_hand_matrix(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!g_armosHammerActive || player == nullptr || player->checkWolf() ||
        player->mSwordModel == nullptr)
    {
        return;
    }

    cMtx_copy(player->mSwordModel->getBaseTRMtx(), g_armosHammerHandMatrix);
    g_armosHammerHandMatrixValid = true;
    if (svc_interp->record_mtx_keyed(
            mod_ctx, g_armosHammerHandMatrix, &g_armosHammerHandMatrix) != MOD_OK &&
        !g_armosHammerInterpolationErrorLogged)
    {
        g_armosHammerInterpolationErrorLogged = true;
        svc_log->error(mod_ctx, "failed to record Armos Hammer hand interpolation matrix");
    }
}

void forget_armos_hammer_hand_matrix() {
    g_armosHammerHandMatrixValid = false;
    if (svc_interp->forget_mtx(mod_ctx, &g_armosHammerHandMatrix) != MOD_OK &&
        !g_armosHammerInterpolationErrorLogged)
    {
        g_armosHammerInterpolationErrorLogged = true;
        svc_log->error(mod_ctx, "failed to release Armos Hammer hand interpolation matrix");
    }
}

void restore_armos_hammer_sword_shapes(J3DModel* expectedModel = nullptr) {
    auto& visibility = g_armosHammerSwordVisibility;
    if (visibility.model == nullptr) {
        return;
    }
    if (expectedModel != nullptr && visibility.model != expectedModel) {
        visibility = {};
        return;
    }

    auto* modelData = visibility.model->getModelData();
    const u16 shapeCount = std::min(visibility.shapeCount, modelData->getShapeNum());
    for (u16 i = 0; i < shapeCount; ++i) {
        auto* shape = modelData->getShapeNodePointer(i);
        if (shape == nullptr) {
            continue;
        }
        if (visibility.wasHidden[i]) {
            shape->hide();
        } else {
            shape->show();
        }
    }
    visibility = {};
}

void set_armos_hammer_sword_visible(daAlink_c* player, bool visible) {
    if (visible) {
        if (player == nullptr) {
            g_armosHammerSwordVisibility = {};
        } else {
            restore_armos_hammer_sword_shapes(player->mSwordModel);
        }
        return;
    }
    if (player == nullptr || player->mSwordModel == nullptr) {
        return;
    }

    auto& visibility = g_armosHammerSwordVisibility;
    if (visibility.model != player->mSwordModel) {
        restore_armos_hammer_sword_shapes();
        auto* modelData = player->mSwordModel->getModelData();
        visibility.model = player->mSwordModel;
        visibility.shapeCount =
            std::min(modelData->getShapeNum(), kArmosHammerMaxSwordShapes);
        for (u16 i = 0; i < visibility.shapeCount; ++i) {
            auto* shape = modelData->getShapeNodePointer(i);
            visibility.wasHidden[i] =
                shape != nullptr && shape->checkFlag(J3DShpFlag_Visible);
        }
    }

    auto* modelData = player->mSwordModel->getModelData();
    for (u16 i = 0; i < modelData->getShapeNum(); ++i) {
        auto* shape = modelData->getShapeNodePointer(i);
        if (shape != nullptr) {
            shape->hide();
        }
    }
}

void draw_armos_hammer_cylinder(
    ArmosHammerCylinderPacket& packet, f32 x, f32 y, f32 z,
    f32 radiusX, f32 halfHeight, f32 radiusZ, s16 rotationZ)
{
    mDoMtx_stack_c::identity();
    mDoMtx_stack_c::transM(x, y, z);
    mDoMtx_stack_c::ZrotM(rotationZ);
    mDoMtx_stack_c::scaleM(radiusX, halfHeight, radiusZ);
    cMtx_copy(mDoMtx_stack_c::get(), packet.mLocalMatrix);
    dComIfGd_getOpaList()->entryImm(&packet, 0);
}

void hide_armos_hammer_sword(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (g_armosHammerActive && player != nullptr && !player->checkWolf()) {
        set_armos_hammer_sword_visible(player, false);
    }
}

void draw_armos_hammer_on_player(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!g_armosHammerActive || player == nullptr || player->checkWolf() ||
        player->mSwordModel == nullptr)
    {
        return;
    }

    if (!g_armosHammerHandMatrixValid) {
        return;
    }

    draw_armos_hammer_cylinder(
        g_armosHammerPackets[0], 33.0f, 0.0f, 0.0f, 6.0f,
        48.0f, 6.0f, 0x4000);
    draw_armos_hammer_cylinder(
        g_armosHammerPackets[1], 8.0f, 0.0f, 0.0f, 7.5f,
        10.0f, 7.5f, 0x4000);
    draw_armos_hammer_cylinder(
        g_armosHammerPackets[2], 75.0f, 0.0f, 0.0f, 7.5f,
        8.0f, 7.5f, 0x4000);

    draw_armos_hammer_cylinder(
        g_armosHammerPackets[3], 98.0f, 0.0f, 0.0f, 25.0f,
        24.0f, 23.0f, 0);
    draw_armos_hammer_cylinder(
        g_armosHammerPackets[4], 98.0f, 0.0f, 0.0f, 27.0f,
        4.0f, 25.0f, 0);
    draw_armos_hammer_cylinder(
        g_armosHammerPackets[5], 98.0f, 0.0f, 0.0f, 26.0f,
        2.0f, 24.0f, 0);
}

void update_armos_hammer() {
    if (!g_armosHammerActive) {
        return;
    }

    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (player == nullptr) {
        g_armosHammerSwordVisibility = {};
        forget_armos_hammer_hand_matrix();
        g_armosHammerActive = false;
        return;
    }
    if (!player->checkWolf()) {
        set_armos_hammer_sword_visible(player, false);
    }
}

void deactivate_armos_hammer_after_scene_change(
    ModContext*, void*, void* retval, void*)
{
    if (retval == nullptr || *static_cast<int*>(retval) == 0) {
        return;
    }

    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (player != nullptr && !player->checkWolf()) {
        set_armos_hammer_sword_visible(player, true);
    } else {
        g_armosHammerSwordVisibility = {};
    }
    forget_armos_hammer_hand_matrix();
    g_armosHammerActive = false;
}

void apply_armos_hammer_attack_type(ModContext*, void*, void* retval, void*) {
    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (g_armosHammerActive && player != nullptr && !player->checkWolf() &&
        retval != nullptr)
    {
        *static_cast<u32*>(retval) |= AT_TYPE_BOMB;
    }
}

DEFINE_HOOK(&daAlink_c::getSwordAtType, ArmosHammerGetSwordAtType);
