#include "features/dark_link_eye_bloom.hpp"

namespace {

DEFINE_HOOK(&J3DMatPacket::draw, DarkLinkMaterialDrawHook);
DEFINE_HOOK(&J3DShapePacket::draw, DarkLinkShapePacketDrawHook);
DEFINE_HOOK(&J3DShapePacket::drawFast, DarkLinkShapePacketFastHook);
DEFINE_HOOK(&J3DShape::drawFast, DarkLinkShapeDrawHook);
DEFINE_HOOK(&J3DShapeDraw::draw, DarkLinkEyePrimitiveDrawHook);
DEFINE_HOOK(&at_power_check, DarkLinkAttackPowerHook);

struct DarkLinkDrawScope {
    J3DShapePacket* packet = nullptr;
    J3DMatPacket* materialPacket = nullptr;
    bool applied = false;
    bool eyeGlow = false;
};

std::array<DarkLinkDrawScope, 16> g_darkLinkDrawScopes{};
std::array<J3DMatPacket*, 16> g_darkLinkMaterialScopes{};
size_t g_darkLinkDrawDepth = 0;
size_t g_darkLinkMaterialDepth = 0;
J3DModel* g_darkLinkRemoteDrawModel = nullptr;

struct DarkLinkSwapTable {
    int identity = -1;
};

DarkLinkSwapTable find_dark_link_swap_table(J3DTevBlock* tev, unsigned stageCount) {
    std::array<bool, 4> used{};
    DarkLinkSwapTable result;
    for (unsigned i = 0; i < stageCount; ++i) {
        const auto* stage = tev->getTevStage(i);
        if (stage == nullptr) {
            return result;
        }

        used[stage->mTevSwapModeInfo & 3] = true;
        used[(stage->mTevSwapModeInfo >> 2) & 3] = true;
    }
    for (int i = 0; i < 4; ++i) {
        const auto* table = tev->getTevSwapModeTable(i);
        if (table != nullptr && table->getR() == GX_CH_RED &&
            table->getG() == GX_CH_GREEN && table->getB() == GX_CH_BLUE &&
            table->getA() == GX_CH_ALPHA)
        {
            result.identity = i;
            break;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (!used[i] && i != result.identity && result.identity < 0) {
            result.identity = i;
        }
    }
    return result;
}

bool dark_link_eye_material(J3DModelData* modelData, J3DMaterial* material) {
    if (modelData == nullptr || material == nullptr) {
        return false;
    }
    auto* names = modelData->getMaterialName();
    if (names == nullptr) {
        return false;
    }
    const char* name = names->getName(material->getIndex());
    return name != nullptr &&
        (std::strstr(name, "eyeballL") != nullptr ||
         std::strstr(name, "eyeballR") != nullptr);
}

class DarkLinkDisplayList {
    alignas(32) std::array<u8, 1024> bytes{};
    size_t size = 0;
    bool valid = true;

    void byte(u8 value) {
        if (size < bytes.size()) {
            bytes[size++] = value;
        } else {
            valid = false;
        }
    }
    void word(u32 value) {
        byte(value >> 24);
        byte(value >> 16);
        byte(value >> 8);
        byte(value);
    }
    void bp(u8 reg, u32 value) {
        byte(0x61);
        word((u32(reg) << 24) | (value & 0xFFFFFF));
    }
    void masked_bp(u8 reg, u32 mask, u32 value) {
        bp(0xFE, mask);
        bp(reg, value);
    }
    void xf(u16 reg, u32 value) {
        byte(0x10);
        word(reg);
        word(value);
    }
    static u32 rgba(GXColor color) {
        return (u32(color.r) << 24) | (u32(color.g) << 16) |
            (u32(color.b) << 8) | color.a;
    }

public:
    void lighting(bool eye, u8 ambientAlpha, u8 materialAlpha) {
        xf(0x1009, 2);
        xf(0x100B, rgba({64, 64, 64, ambientAlpha}));
        xf(0x100D, rgba(eye ? GXColor{255, 28, 20, materialAlpha}
                            : GXColor{8, 9, 11, materialAlpha}));
        xf(0x100F, (1u << 10) | (u32(GX_DF_CLAMP) << 7) |
            (eye ? 0u : (1u << 1) | (1u << 2)));
    }

    void eye_mask() {
        masked_bp(0x41, 0xFE3u, 1u | (4u << 8));
        masked_bp(0x40, 0x1Fu, 1u | (3u << 1) | (1u << 4));
    }

    void specular_lighting(u8 ambientAlpha0, u8 materialAlpha0,
                           u8 ambientAlpha1, u8 materialAlpha1) {
        xf(0x1009, 2);
        xf(0x100A, rgba({64, 64, 64, ambientAlpha0}));
        xf(0x100C, rgba({8, 9, 11, materialAlpha0}));
        xf(0x100E, (1u << 10) | (u32(GX_DF_CLAMP) << 7) | (1u << 1) | (1u << 2));
        xf(0x100B, rgba({0, 0, 0, ambientAlpha1}));
        xf(0x100D, rgba({24, 28, 32, materialAlpha1}));
        xf(0x100F, (1u << 9) | (1u << 1) | (1u << 2));

        xf(0x0603, rgba({255, 255, 255, 255}));
        const float light[12] = {
            0.0f, 0.0f, 1.0f, 16.0f, 0.0f, -15.0f,
            -419430.4f, 524288.0f, 805414.91f,
            -0.212703f, 0.265879f, 0.940248f,
        };
        for (u16 i = 0; i < std::size(light); ++i) {
            u32 bits;
            std::memcpy(&bits, &light[i], sizeof(bits));
            xf(0x0604 + i, bits);
        }
    }

    void swap(unsigned table) {
        masked_bp(0xF6 + 2 * table, 0xF, GX_CH_RED | (GX_CH_GREEN << 2));
        masked_bp(0xF7 + 2 * table, 0xF, GX_CH_BLUE | (GX_CH_ALPHA << 2));
    }

    void stage(unsigned index, unsigned identity, GXTevColorArg a, GXTevColorArg b,
               GXTevColorArg c, GXTevColorArg d, GXTevScale scale = GX_CS_SCALE_1,
               unsigned rasterChannel = 1) {
        const unsigned shift = (index & 1) * 12;
        const u32 order = rasterChannel << 7;
        masked_bp(0x28 + index / 2, 0xFFFu << shift, order << shift);
        bp(0x10 + index, 0);
        bp(0xC0 + 2 * index, u32(d) | (u32(c) << 4) | (u32(b) << 8) |
            (u32(a) << 12) | (1u << 19) | (u32(scale) << 20));
        bp(0xC1 + 2 * index, identity | (identity << 2) |
            (u32(GX_CA_APREV) << 4) | (u32(GX_CA_ZERO) << 7) |
            (u32(GX_CA_ZERO) << 10) | (u32(GX_CA_ZERO) << 13) | (1u << 19));
    }

    bool submit() {
        while (size % 32 != 0 && valid) {
            byte(0);
        }
        if (!valid) {
            return false;
        }
        GXCallDisplayList(bytes.data(), static_cast<u32>(size));
        return true;
    }

    bool apply(unsigned stageCount, unsigned texGenCount) {
        masked_bp(0x00, 0x3C7F, texGenCount | (2u << 4) | ((stageCount - 1) << 10));
        xf(0x103F, texGenCount);
        return submit();
    }
};

bool is_dark_link_model(daAlink_c* link, J3DModel* model) {
    return link != nullptr && model != nullptr && !link->checkWolf() &&
        link->mClothesChangeWaitTimer == 0 &&
        (model == link->mpLinkModel || model == link->mpLinkFaceModel ||
         model == link->mpLinkHatModel ||          model == link->mpLinkHandModel ||
         model == link->mpLinkBootModels[0] || model == link->mpLinkBootModels[1] ||
         model == link->mShieldModel || model == link->mSwordModel ||
         model == link->mSheathModel);
}

J3DMatPacket* dark_link_drawing_material(J3DShapePacket* packet) {
    if (g_darkLinkMaterialDepth == 0 ||
        g_darkLinkMaterialDepth > g_darkLinkMaterialScopes.size())
    {
        return nullptr;
    }
    auto* material = g_darkLinkMaterialScopes[g_darkLinkMaterialDepth - 1];
    if (material == nullptr) {
        return nullptr;
    }
    for (auto* shape = material->getShapePacket(); shape != nullptr;
         shape = static_cast<J3DShapePacket*>(shape->getNextPacket()))
    {
        if (shape == packet) {
            return material;
        }
    }
    return nullptr;
}

HookAction begin_dark_link_material_draw(ModContext*, void* args, void*, void*) {
    if (g_darkLinkMaterialDepth < g_darkLinkMaterialScopes.size()) {
        g_darkLinkMaterialScopes[g_darkLinkMaterialDepth] =
            mods::arg<J3DMatPacket*>(args, 0);
    }
    ++g_darkLinkMaterialDepth;
    return HOOK_CONTINUE;
}

void end_dark_link_material_draw(ModContext*, void*, void*, void*) {
    if (g_darkLinkMaterialDepth == 0) {
        return;
    }
    --g_darkLinkMaterialDepth;
    if (g_darkLinkMaterialDepth < g_darkLinkMaterialScopes.size()) {
        g_darkLinkMaterialScopes[g_darkLinkMaterialDepth] = nullptr;
    }
}

HookAction begin_dark_link_shape_packet(ModContext*, void* args, void*, void*) {
    if (g_darkLinkDrawDepth < g_darkLinkDrawScopes.size()) {
        auto& scope = g_darkLinkDrawScopes[g_darkLinkDrawDepth];
        scope = {};
        auto* packet = mods::arg<J3DShapePacket*>(args, 0);
        auto* model = packet != nullptr ? packet->getModel() : nullptr;
        const bool localDarkLink = g_darkLinkVisualActive &&
            is_dark_link_model(daAlink_getAlinkActorClass(), model);
        const bool remoteDarkLink = model != nullptr &&
            model == g_darkLinkRemoteDrawModel;
        if (localDarkLink || remoteDarkLink)
        {
            scope.materialPacket = dark_link_drawing_material(packet);
            if (scope.materialPacket != nullptr) {
                scope.packet = packet;
            }
        }
    }
    ++g_darkLinkDrawDepth;
    return HOOK_CONTINUE;
}

void restore_dark_link_draw_state(DarkLinkDrawScope& scope) {
    if (!scope.applied || scope.packet == nullptr || scope.materialPacket == nullptr) {
        return;
    }
    scope.materialPacket->callDL();
    if (scope.packet->getDisplayListObj() != nullptr) {
        scope.packet->getDisplayListObj()->callDL();
    }
    scope.applied = false;
}

void end_dark_link_shape_packet(ModContext*, void*, void*, void*) {
    if (g_darkLinkDrawDepth == 0) {
        return;
    }
    --g_darkLinkDrawDepth;
    if (g_darkLinkDrawDepth < g_darkLinkDrawScopes.size()) {
        restore_dark_link_draw_state(g_darkLinkDrawScopes[g_darkLinkDrawDepth]);
        g_darkLinkDrawScopes[g_darkLinkDrawDepth] = {};
    }
}

HookAction apply_dark_link_appearance(ModContext*, void* args, void*, void*) {
    if (g_darkLinkDrawDepth == 0 ||
        g_darkLinkDrawDepth > g_darkLinkDrawScopes.size())
    {
        return HOOK_CONTINUE;
    }
    auto& scope = g_darkLinkDrawScopes[g_darkLinkDrawDepth - 1];
    auto* shape = mods::arg<const J3DShape*>(args, 0);
    if (scope.packet == nullptr || scope.materialPacket == nullptr || shape == nullptr ||
        scope.applied || scope.packet->getShape() != shape ||
        scope.materialPacket->getDisplayListObj() == nullptr)
    {
        return HOOK_CONTINUE;
    }
    auto* material = shape->getMaterial();
    auto* model = scope.packet->getModel();
    if (material == nullptr || model == nullptr ||
        material->getTevBlock() == nullptr || material->getColorBlock() == nullptr ||
        material->getTexGenBlock() == nullptr)
    {
        return HOOK_CONTINUE;
    }

    auto* tev = material->getTevBlock();
    const unsigned count = tev->getTevStageNum();
    if (count == 0 || count >= 16) {
        return HOOK_CONTINUE;
    }
    const auto swaps = find_dark_link_swap_table(tev, count);
    const unsigned texGenCount = material->getTexGenNum();
    if (swaps.identity < 0 || texGenCount > 8) {
        return HOOK_CONTINUE;
    }
    const bool eye = dark_link_eye_material(model->getModelData(), material);
    scope.eyeGlow = eye;
    const auto* ambient = material->getColorBlock()->getAmbColor(1);
    const auto* color = material->getMatColor(1);
    const u8 ambientAlpha = ambient != nullptr ? ambient->a : 255;
    const u8 materialAlpha = color != nullptr ? color->a : 255;
    auto* batchMaterial = scope.materialPacket->getMaterial();
    const auto* alpha0 = material->getColorBlock()->getColorChan(1);
    const auto* alpha1 = material->getColorBlock()->getColorChan(3);
    const bool alphaUsesLight0 =
        (alpha0 != nullptr && alpha0->getEnable() && (alpha0->getLightMask() & 1)) ||
        (alpha1 != nullptr && alpha1->getEnable() && (alpha1->getLightMask() & 1));
    const bool glossy = !eye && !alphaUsesLight0 && count + 2 <= 16 &&
        batchMaterial != nullptr && batchMaterial->getColorBlock() != nullptr &&
        batchMaterial->getColorBlock()->getLight(0) != nullptr;

    DarkLinkDisplayList display;
    display.swap(static_cast<unsigned>(swaps.identity));
    unsigned next = count;
    if (glossy) {
        const auto* ambient0 = material->getColorBlock()->getAmbColor(0);
        const auto* color0 = material->getMatColor(0);
        display.specular_lighting(
            ambient0 != nullptr ? ambient0->a : 255,
            color0 != nullptr ? color0->a : 255,
            ambientAlpha, materialAlpha);
        display.stage(count, swaps.identity, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
                      GX_CC_RASC, GX_CS_SCALE_1, 0);
        display.stage(count + 1, swaps.identity, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO,
                      GX_CC_CPREV);
        next += 2;
    } else {
        display.lighting(eye, ambientAlpha, materialAlpha);
        display.stage(count, swaps.identity, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
                      GX_CC_RASC, eye ? GX_CS_SCALE_2 : GX_CS_SCALE_1);
        ++next;
    }
    scope.applied = display.apply(next, texGenCount);
    return HOOK_CONTINUE;
}

void capture_dark_link_eye_primitive(ModContext*, void* args, void*, void*) {
    if (g_darkLinkDrawDepth == 0 ||
        g_darkLinkDrawDepth > g_darkLinkDrawScopes.size())
    {
        return;
    }
    auto& scope = g_darkLinkDrawScopes[g_darkLinkDrawDepth - 1];
    if (!scope.applied || !scope.eyeGlow || scope.packet == nullptr) {
        return;
    }
    auto* primitive = mods::arg<const J3DShapeDraw*>(args, 0);
    if (primitive == nullptr || primitive->getDisplayList() == nullptr ||
        primitive->getDisplayListSize() == 0 ||
        !dark_link_eye_bloom::begin_capture())
    {
        return;
    }
    DarkLinkDisplayList mask;
    mask.eye_mask();
    if (mask.submit()) {
        GXCallDisplayList(
            primitive->getDisplayList(), primitive->getDisplayListSize());
    }
    dark_link_eye_bloom::end_capture();
    restore_dark_link_draw_state(scope);
    void* shapeArgs[]{scope.packet->getShape()};
    apply_dark_link_appearance(nullptr, shapeArgs, nullptr, nullptr);
}

void end_dark_link_shape_draw(ModContext*, void*, void*, void*) {
    if (g_darkLinkDrawDepth != 0 &&
        g_darkLinkDrawDepth <= g_darkLinkDrawScopes.size())
    {
        restore_dark_link_draw_state(
            g_darkLinkDrawScopes[g_darkLinkDrawDepth - 1]);
    }
}

void apply_mask_damage_multiplier(ModContext*, void* args, void*, void*) {
    auto* attack = mods::arg<dCcU_AtInfo*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    constexpr u32 kSwordAttackTypes =
        AT_TYPE_NORMAL_SWORD | AT_TYPE_MASTER_SWORD;
    if (link == nullptr || attack == nullptr ||
        attack->mpActor != link || attack->mpCollider == nullptr ||
        attack->mHitType != HIT_TYPE_LINK_NORMAL_ATTACK ||
        attack->mAttackPower == 0 ||
        (attack->mpCollider->GetAtType() & kSwordAttackTypes) == 0)
    {
        return;
    }

    int64_t percent = 100;
    if (g_darkLinkVisualActive) {
        percent = darkLinkDamageMultiplierPercent();
    } else if (g_giantMaskActive && !link->checkWolf()) {
        percent = giantMaskDamageMultiplierPercent();
    } else {
        return;
    }
    attack->mAttackPower = static_cast<u16>(std::min<u32>(
        (static_cast<u32>(attack->mAttackPower) *
             static_cast<u32>(percent) + 99U) / 100U,
        0xFFFFU));
}

ModResult initialize_dark_link_visual_hooks(ModError*) {
    ModResult result;
    if ((result = mods::hook::add_post<DarkLinkAttackPowerHook>(
             svc_hook, apply_mask_damage_multiplier)) != MOD_OK ||
        (result = mods::hook::add_pre<DarkLinkMaterialDrawHook>(
             svc_hook, begin_dark_link_material_draw)) != MOD_OK ||
        (result = mods::hook::add_post<DarkLinkMaterialDrawHook>(
             svc_hook, end_dark_link_material_draw)) != MOD_OK ||
        (result = mods::hook::add_pre<DarkLinkShapePacketDrawHook>(
             svc_hook, begin_dark_link_shape_packet)) != MOD_OK ||
        (result = mods::hook::add_post<DarkLinkShapePacketDrawHook>(
             svc_hook, end_dark_link_shape_packet)) != MOD_OK ||
        (result = mods::hook::add_pre<DarkLinkShapePacketFastHook>(
             svc_hook, begin_dark_link_shape_packet)) != MOD_OK ||
        (result = mods::hook::add_post<DarkLinkShapePacketFastHook>(
             svc_hook, end_dark_link_shape_packet)) != MOD_OK ||
        (result = mods::hook::add_pre<DarkLinkShapeDrawHook>(
             svc_hook, apply_dark_link_appearance)) != MOD_OK ||
        (result = mods::hook::add_post<DarkLinkShapeDrawHook>(
             svc_hook, end_dark_link_shape_draw)) != MOD_OK ||
        (result = mods::hook::add_post<DarkLinkEyePrimitiveDrawHook>(
             svc_hook, capture_dark_link_eye_primitive)) != MOD_OK)
    {
        mods::hook::uninstall<DarkLinkAttackPowerHook>(svc_hook);
        mods::hook::uninstall<DarkLinkMaterialDrawHook>(svc_hook);
        mods::hook::uninstall<DarkLinkShapePacketDrawHook>(svc_hook);
        mods::hook::uninstall<DarkLinkShapePacketFastHook>(svc_hook);
        mods::hook::uninstall<DarkLinkShapeDrawHook>(svc_hook);
        mods::hook::uninstall<DarkLinkEyePrimitiveDrawHook>(svc_hook);
        svc_log->error(mod_ctx, "failed to install Dawnlight Dark Link material hooks");
        return result;
    }
    dark_link_eye_bloom::initialize();
    return MOD_OK;
}

void shutdown_dark_link_visual_hooks() {
    dark_link_eye_bloom::shutdown();
    mods::hook::uninstall<DarkLinkAttackPowerHook>(svc_hook);
    mods::hook::uninstall<DarkLinkMaterialDrawHook>(svc_hook);
    mods::hook::uninstall<DarkLinkShapePacketDrawHook>(svc_hook);
    mods::hook::uninstall<DarkLinkShapePacketFastHook>(svc_hook);
    mods::hook::uninstall<DarkLinkShapeDrawHook>(svc_hook);
    mods::hook::uninstall<DarkLinkEyePrimitiveDrawHook>(svc_hook);
    g_darkLinkDrawScopes.fill({});
    g_darkLinkMaterialScopes.fill(nullptr);
    g_darkLinkDrawDepth = 0;
    g_darkLinkMaterialDepth = 0;
}

}  // namespace
