void tint_tunic_icon(J2DPicture* icon) {
    if (icon != nullptr) {
        icon->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
            JUtility::TColor(190, 24, 24, 255));
    }
}

void tint_tunic_icon(CPaneMgr* icon) {
    if (icon != nullptr) {
        icon->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
            JUtility::TColor(190, 24, 24, 255));
    }
}

void tint_secondary_ring_icons(dMenu_Ring_c* ring) {
    if (!isSecondaryRing(ring)) {
        return;
    }

    for (auto* icon : g_tunicWheelItemTex) {
        tint_tunic_icon(icon);
    }

    for (int button = 0; button < 2; ++button) {
        if (!secondaryTunicAssigned(button)) {
            continue;
        }
        for (auto* icon : ring->mpSelectItemTex[button]) {
            tint_tunic_icon(icon);
        }
    }
}

void clear_tunic_wheel_textures() {
    for (auto*& picture : g_tunicWheelItemTex) {
        if (picture != nullptr) {
            JKR_DELETE(picture);
            picture = nullptr;
        }
    }
    if (g_tunicWheelItemHeap != nullptr) {
        for (auto*& buffer : g_tunicWheelItemBuf) {
            if (buffer != nullptr) {
                g_tunicWheelItemHeap->free(buffer);
                buffer = nullptr;
            }
        }
    }
    g_tunicWheelItemHeap = nullptr;
}

void clear_sit_wheel_texture() {
    if (g_sitWheelItemTex != nullptr) {
        JKR_DELETE(g_sitWheelItemTex);
        g_sitWheelItemTex = nullptr;
    }
    if (g_sitWheelItemHeap != nullptr && g_sitWheelItemBuf != nullptr) {
        g_sitWheelItemHeap->free(g_sitWheelItemBuf);
    }
    g_sitWheelItemBuf = nullptr;
    g_sitWheelItemHeap = nullptr;
}

void clear_ice_leaf_wheel_texture() {
    if (g_iceLeafWheelItemTex != nullptr) {
        JKR_DELETE(g_iceLeafWheelItemTex);
        g_iceLeafWheelItemTex = nullptr;
    }
    if (g_iceLeafWheelItemHeap != nullptr && g_iceLeafWheelItemBuf != nullptr) {
        g_iceLeafWheelItemHeap->free(g_iceLeafWheelItemBuf);
    }
    g_iceLeafWheelItemBuf = nullptr;
    g_iceLeafWheelItemHeap = nullptr;
}

void clear_lens_of_truth_wheel_texture() {
    if (g_lensOfTruthWheelItemTex != nullptr) {
        JKR_DELETE(g_lensOfTruthWheelItemTex);
        g_lensOfTruthWheelItemTex = nullptr;
    }
    if (g_lensOfTruthWheelItemHeap != nullptr &&
        g_lensOfTruthWheelItemBuf != nullptr)
    {
        g_lensOfTruthWheelItemHeap->free(g_lensOfTruthWheelItemBuf);
    }
    g_lensOfTruthWheelItemBuf = nullptr;
    g_lensOfTruthWheelItemHeap = nullptr;
}

void clear_goron_mask_wheel_texture() {
    if (g_goronMaskWheelItemTex != nullptr) {
        JKR_DELETE(g_goronMaskWheelItemTex);
        g_goronMaskWheelItemTex = nullptr;
    }
    if (g_goronMaskWheelItemHeap != nullptr &&
        g_goronMaskWheelItemBuf != nullptr)
    {
        g_goronMaskWheelItemHeap->free(g_goronMaskWheelItemBuf);
    }
    g_goronMaskWheelItemBuf = nullptr;
    g_goronMaskWheelItemHeap = nullptr;
}

void clear_giant_mask_wheel_texture() {
    if (g_giantMaskWheelItemTex != nullptr) {
        JKR_DELETE(g_giantMaskWheelItemTex);
        g_giantMaskWheelItemTex = nullptr;
    }
    if (g_giantMaskWheelItemHeap != nullptr &&
        g_giantMaskWheelItemBuf != nullptr)
    {
        g_giantMaskWheelItemHeap->free(g_giantMaskWheelItemBuf);
    }
    g_giantMaskWheelItemBuf = nullptr;
    g_giantMaskWheelItemHeap = nullptr;
}

void clear_armos_hammer_wheel_texture() {
    if (g_armosHammerWheelItemTex != nullptr) {
        JKR_DELETE(g_armosHammerWheelItemTex);
        g_armosHammerWheelItemTex = nullptr;
    }
    if (g_armosHammerWheelItemHeap != nullptr &&
        g_armosHammerWheelItemBuf != nullptr)
    {
        g_armosHammerWheelItemHeap->free(g_armosHammerWheelItemBuf);
    }
    g_armosHammerWheelItemBuf = nullptr;
    g_armosHammerWheelItemHeap = nullptr;
}

void load_lens_of_truth_wheel_texture(dMenu_Ring_c* ring) {
    clear_lens_of_truth_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kLensOfTruthIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Lens of Truth wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Lens of Truth wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_lensOfTruthWheelItemHeap = ring->mpHeap;
    g_lensOfTruthWheelItemBuf = static_cast<ResTIMG*>(
        g_lensOfTruthWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_lensOfTruthWheelItemBuf == nullptr) {
        g_lensOfTruthWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Lens of Truth wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_lensOfTruthWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_lensOfTruthWheelItemBuf, kItemTextureBufferSize);

    g_lensOfTruthWheelItemTex = JKR_NEW J2DPicture(g_lensOfTruthWheelItemBuf);
    if (g_lensOfTruthWheelItemTex == nullptr) {
        g_lensOfTruthWheelItemHeap->free(g_lensOfTruthWheelItemBuf);
        g_lensOfTruthWheelItemBuf = nullptr;
        g_lensOfTruthWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Lens of Truth wheel icon picture");
        return;
    }
    g_lensOfTruthWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void load_goron_mask_wheel_texture(dMenu_Ring_c* ring) {
    clear_goron_mask_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kGoronMaskIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Goron Mask wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Goron Mask wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_goronMaskWheelItemHeap = ring->mpHeap;
    g_goronMaskWheelItemBuf = static_cast<ResTIMG*>(
        g_goronMaskWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_goronMaskWheelItemBuf == nullptr) {
        g_goronMaskWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Goron Mask wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_goronMaskWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_goronMaskWheelItemBuf, kItemTextureBufferSize);

    g_goronMaskWheelItemTex = JKR_NEW J2DPicture(g_goronMaskWheelItemBuf);
    if (g_goronMaskWheelItemTex == nullptr) {
        g_goronMaskWheelItemHeap->free(g_goronMaskWheelItemBuf);
        g_goronMaskWheelItemBuf = nullptr;
        g_goronMaskWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Goron Mask wheel icon picture");
        return;
    }
    g_goronMaskWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void load_giant_mask_wheel_texture(dMenu_Ring_c* ring) {
    clear_giant_mask_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kGiantMaskIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Giant's Mask wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Giant's Mask wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_giantMaskWheelItemHeap = ring->mpHeap;
    g_giantMaskWheelItemBuf = static_cast<ResTIMG*>(
        g_giantMaskWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_giantMaskWheelItemBuf == nullptr) {
        g_giantMaskWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Giant's Mask wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_giantMaskWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_giantMaskWheelItemBuf, kItemTextureBufferSize);

    g_giantMaskWheelItemTex = JKR_NEW J2DPicture(g_giantMaskWheelItemBuf);
    if (g_giantMaskWheelItemTex == nullptr) {
        g_giantMaskWheelItemHeap->free(g_giantMaskWheelItemBuf);
        g_giantMaskWheelItemBuf = nullptr;
        g_giantMaskWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Giant's Mask wheel icon picture");
        return;
    }
    g_giantMaskWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void load_armos_hammer_wheel_texture(dMenu_Ring_c* ring) {
    clear_armos_hammer_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kArmosHammerIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Armos Hammer wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Armos Hammer wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_armosHammerWheelItemHeap = ring->mpHeap;
    g_armosHammerWheelItemBuf = static_cast<ResTIMG*>(
        g_armosHammerWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_armosHammerWheelItemBuf == nullptr) {
        g_armosHammerWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Armos Hammer wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_armosHammerWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_armosHammerWheelItemBuf, kItemTextureBufferSize);

    g_armosHammerWheelItemTex = JKR_NEW J2DPicture(g_armosHammerWheelItemBuf);
    if (g_armosHammerWheelItemTex == nullptr) {
        g_armosHammerWheelItemHeap->free(g_armosHammerWheelItemBuf);
        g_armosHammerWheelItemBuf = nullptr;
        g_armosHammerWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Armos Hammer wheel icon picture");
        return;
    }
    g_armosHammerWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void clear_shield_surf_wheel_textures() {
    for (auto*& picture : g_shieldSurfWheelItemTex) {
        if (picture != nullptr) {
            JKR_DELETE(picture);
            picture = nullptr;
        }
    }
    if (g_shieldSurfWheelItemHeap != nullptr) {
        for (auto*& buffer : g_shieldSurfWheelItemBuf) {
            if (buffer != nullptr) {
                g_shieldSurfWheelItemHeap->free(buffer);
                buffer = nullptr;
            }
        }
    }
    g_shieldSurfWheelItemHeap = nullptr;
}

void load_sit_wheel_texture(dMenu_Ring_c* ring) {
    clear_sit_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kSitIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Sit wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Sit wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_sitWheelItemHeap = ring->mpHeap;
    g_sitWheelItemBuf =
        static_cast<ResTIMG*>(g_sitWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_sitWheelItemBuf == nullptr) {
        g_sitWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Sit wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_sitWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_sitWheelItemBuf, kItemTextureBufferSize);

    g_sitWheelItemTex = JKR_NEW J2DPicture(g_sitWheelItemBuf);
    if (g_sitWheelItemTex == nullptr) {
        g_sitWheelItemHeap->free(g_sitWheelItemBuf);
        g_sitWheelItemBuf = nullptr;
        g_sitWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Sit wheel icon picture");
        return;
    }
    g_sitWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void load_ice_leaf_wheel_texture(dMenu_Ring_c* ring) {
    clear_ice_leaf_wheel_texture();
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kIceLeafIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Ice Leaf wheel icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Ice Leaf wheel icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    g_iceLeafWheelItemHeap = ring->mpHeap;
    g_iceLeafWheelItemBuf = static_cast<ResTIMG*>(
        g_iceLeafWheelItemHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_iceLeafWheelItemBuf == nullptr) {
        g_iceLeafWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to allocate the Ice Leaf wheel icon buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    std::memcpy(g_iceLeafWheelItemBuf, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_iceLeafWheelItemBuf, kItemTextureBufferSize);

    g_iceLeafWheelItemTex = JKR_NEW J2DPicture(g_iceLeafWheelItemBuf);
    if (g_iceLeafWheelItemTex == nullptr) {
        g_iceLeafWheelItemHeap->free(g_iceLeafWheelItemBuf);
        g_iceLeafWheelItemBuf = nullptr;
        g_iceLeafWheelItemHeap = nullptr;
        svc_log->error(mod_ctx, "failed to create the Ice Leaf wheel icon picture");
        return;
    }
    g_iceLeafWheelItemTex->setBasePosition(J2DBasePosition_4);
}

void load_shield_surf_wheel_textures(dMenu_Ring_c* ring) {
    clear_shield_surf_wheel_textures();
    g_shieldSurfWheelItemHeap = ring->mpHeap;
    for (auto*& buffer : g_shieldSurfWheelItemBuf) {
        buffer = static_cast<ResTIMG*>(ring->mpHeap->alloc(kItemTextureBufferSize, 0x20));
        if (buffer == nullptr) {
            svc_log->error(mod_ctx, "failed to allocate the Shield Surf wheel icon");
            clear_shield_surf_wheel_textures();
            return;
        }
    }

    const u8 shieldItem = shield_surf_display_item();
    const int textureCount = dMeter2Info_readItemTexture(
        shieldItem, g_shieldSurfWheelItemBuf[0], nullptr, g_shieldSurfWheelItemBuf[1],
        nullptr, g_shieldSurfWheelItemBuf[2], nullptr, nullptr, nullptr, -1);
    if (textureCount <= 0) {
        svc_log->error(mod_ctx, "failed to read the Shield Surf wheel icon");
        clear_shield_surf_wheel_textures();
        return;
    }
    for (int texture = 0; texture < std::min(textureCount, 3); ++texture) {
        g_shieldSurfWheelItemTex[texture] =
            JKR_NEW J2DPicture(g_shieldSurfWheelItemBuf[texture]);
        if (g_shieldSurfWheelItemTex[texture] == nullptr) {
            svc_log->error(mod_ctx, "failed to create the Shield Surf wheel icon");
            clear_shield_surf_wheel_textures();
            return;
        }
        g_shieldSurfWheelItemTex[texture]->setBasePosition(J2DBasePosition_4);
    }
    dMeter2Info_setItemColor(
        shieldItem, g_shieldSurfWheelItemTex[0], g_shieldSurfWheelItemTex[1],
        g_shieldSurfWheelItemTex[2], nullptr);
}

void load_tunic_wheel_textures(dMenu_Ring_c* ring) {
    clear_tunic_wheel_textures();
    g_tunicWheelItemHeap = ring->mpHeap;
    for (auto*& buffer : g_tunicWheelItemBuf) {
        buffer = static_cast<ResTIMG*>(ring->mpHeap->alloc(0xC00, 0x20));
    }

    const int textureCount = dMeter2Info_readItemTexture(
        kGoronTunicDisplayItem, g_tunicWheelItemBuf[0], nullptr, g_tunicWheelItemBuf[1], nullptr,
        g_tunicWheelItemBuf[2], nullptr, nullptr, nullptr, -1);
    for (int texture = 0; texture < std::min(textureCount, 3); ++texture) {
        g_tunicWheelItemTex[texture] = JKR_NEW J2DPicture(g_tunicWheelItemBuf[texture]);
        g_tunicWheelItemTex[texture]->setBasePosition(J2DBasePosition_4);
    }
    dMeter2Info_setItemColor(kGoronTunicDisplayItem, g_tunicWheelItemTex[0],
        g_tunicWheelItemTex[1], g_tunicWheelItemTex[2], nullptr);
    tint_secondary_ring_icons(ring);
}

void record_secondary_ring_pose(dMenu_Ring_c* ring);

void setup_secondary_ring(dMenu_Ring_c* ring) {
    g_secondaryWheelSlotIds.fill(dItemNo_NONE_e);
    for (u8 slot = 0; slot < MAX_ITEM_SLOTS; ++slot) {
        g_secondaryWheelSlotIds[slot] = kSecondarySlotBase + slot;
    }
    g_secondaryWheelSlotIds[MAX_ITEM_SLOTS] = kIceLeafSlot;
    g_secondaryWheelSlotIds[MAX_ITEM_SLOTS + 1] = kShieldSurfSlot;
    g_secondaryWheelSlotIds[MAX_ITEM_SLOTS + 2] = kSitSlot;
    g_secondaryWheelSlotIds[MAX_ITEM_SLOTS + 3] = kGoronTunicSlot;

    ring->mItemsTotal = MAX_ITEM_SLOTS;
    ring->mTotalItemTexToAlloc = MAX_ITEM_SLOTS;
    ring->field_0x634 = 0x10000 / kSecondaryWheelSlotCount;
    ring->mCurrentSlot = 0;
    ring->mXButtonSlot = dItemNo_NONE_e;
    ring->mYButtonSlot = dItemNo_NONE_e;
    ring->field_0x6ac = dItemNo_NONE_e;
    ring->field_0x6b4[kZItemSelectIndex] = dItemNo_NONE_e;
    ring->field_0x6b8[kZItemSelectIndex] = dItemNo_NONE_e;
    ring->mStatus = dMenu_Ring_c::STATUS_WAIT;
    ring->mOldStatus = dMenu_Ring_c::STATUS_WAIT;
    ring->mWaitFrames = std::max<s16>(g_ringHIO.mCursorChangeWaitFrames, 0);
    ring->field_0x674[0] = 0;
    ring->field_0x674[1] = 0;
    ring->field_0x674[2] = 0;
    ring->field_0x674[3] = 0;
    ring->mRingRadiusH = g_ringHIO.mRingRadiusH;
    ring->mRingRadiusV = g_ringHIO.mRingRadiusV;
    ring->mAlphaRate = 0.0f;
    ring->mCenterPosX = 0.0f;
    ring->mCenterPosY = 0.0f;
    if (ring->mRingOrigin == 0) {
        ring->mCenterPosY = FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 2) {
        ring->mCenterPosY = -FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 3) {
        ring->mCenterPosX = FB_WIDTH_BASE;
    } else {
        ring->mCenterPosX = -FB_WIDTH_BASE;
    }

    const f32 centerX = g_ringHIO.mItemRingPosX + FB_WIDTH_BASE / 2;
    const f32 centerY = g_ringHIO.mItemRingPosY + FB_HEIGHT_BASE / 2;
    constexpr f32 kTau = 6.28318530717958647692f;
    constexpr int kEllipseSamples = 4096;
    std::array<f32, kEllipseSamples + 1> arcLengths{};
    f32 previousX = 0.0f;
    f32 previousY = -ring->mRingRadiusV;
    for (int sample = 1; sample <= kEllipseSamples; ++sample) {
        const f32 angle = kTau * sample / kEllipseSamples;
        const f32 x = ring->mRingRadiusH * std::sin(angle);
        const f32 y = -ring->mRingRadiusV * std::cos(angle);
        const f32 deltaX = x - previousX;
        const f32 deltaY = y - previousY;
        arcLengths[sample] =
            arcLengths[sample - 1] + std::sqrt(deltaX * deltaX + deltaY * deltaY);
        previousX = x;
        previousY = y;
    }

    const f32 perimeter = arcLengths[kEllipseSamples];
    for (u8 index = 0; index < kSecondaryWheelSlotCount; ++index) {
        const f32 targetDistance = perimeter * index / kSecondaryWheelSlotCount;
        int sample = 1;
        while (sample < kEllipseSamples &&
            arcLengths[sample] < targetDistance)
        {
            ++sample;
        }
        const f32 segmentLength = arcLengths[sample] - arcLengths[sample - 1];
        const f32 segmentFraction = segmentLength > 0.0f
            ? (targetDistance - arcLengths[sample - 1]) / segmentLength
            : 0.0f;
        const f32 angle = kTau * (sample - 1 + segmentFraction) / kEllipseSamples;
        g_secondaryWheelSlotPosX[index] =
            centerX + ring->mRingRadiusH * std::sin(angle);
        g_secondaryWheelSlotPosY[index] =
            centerY - ring->mRingRadiusV * std::cos(angle);
        g_secondaryWheelSlotAngles[index] = cM_atan2s(
            g_secondaryWheelSlotPosX[index] - centerX,
            g_secondaryWheelSlotPosY[index] - centerY);
    }

    for (int i = 0; i < MAX_ITEM_SLOTS; ++i) {
        ring->mItemSlots[i] = dItemNo_NONE_e;
        ring->mItemSlotPosX[i] = g_secondaryWheelSlotPosX[i];
        ring->mItemSlotPosY[i] = g_secondaryWheelSlotPosY[i];
        ring->field_0x63e[i] = g_secondaryWheelSlotAngles[i];
        ring->mItemSlotParam1[i] = 0.0f;
        ring->mItemSlotParam2[i] = 0.0f;
        for (int j = 0; j < 3; ++j) {
            if (ring->mpItemTex[i][j] != nullptr) {
                JKR_DELETE(ring->mpItemTex[i][j]);
                ring->mpItemTex[i][j] = nullptr;
            }
            if (ring->mpItemBuf[i][j] != nullptr) {
                ring->mpHeap->free(ring->mpItemBuf[i][j]);
                ring->mpItemBuf[i][j] = nullptr;
            }
        }
    }

    for (u8 index = 0; index < MAX_ITEM_SLOTS; ++index) {
        const u8 item = secondaryItemIdForSlot(g_secondaryWheelSlotIds[index]);
        const u8 iconItem = secondaryDisplayItemForSlot(g_secondaryWheelSlotIds[index]);
        ring->mItemSlots[index] = g_secondaryWheelSlotIds[index];
        if (item == dItemNo_NONE_e) {
            continue;
        }
        for (int texture = 0; texture < 3; ++texture) {
            ring->mpItemBuf[index][texture] =
                static_cast<ResTIMG*>(ring->mpHeap->alloc(0xC00, 0x20));
        }

        const SecondaryIconContext previousIconContext = g_secondaryIconContext;
        g_secondaryIconContext =
            secondaryIconContextForSlot(g_secondaryWheelSlotIds[index]);
        const int textureCount = dMeter2Info_readItemTexture(
            iconItem, ring->mpItemBuf[index][0], nullptr, ring->mpItemBuf[index][1], nullptr,
            ring->mpItemBuf[index][2], nullptr, nullptr, nullptr,
            isHawkGrassSlot(g_secondaryWheelSlotIds[index]) ? kHawkGrassIconTexture : -1);
        g_secondaryIconContext = previousIconContext;
        for (int texture = 0; texture < std::min(textureCount, 3); ++texture) {
            ring->mpItemTex[index][texture] = JKR_NEW J2DPicture(ring->mpItemBuf[index][texture]);
            ring->mpItemTex[index][texture]->setBasePosition(J2DBasePosition_4);
        }
        dMeter2Info_setItemColor(iconItem, ring->mpItemTex[index][0], ring->mpItemTex[index][1],
            ring->mpItemTex[index][2], nullptr);
        const f32 scale = dItem_data::getTexScale(iconItem) / 100.0f;
        ring->mItemSlotParam1[index] =
            ring->mpItemBuf[index][0]->width / 48.0f * scale * kSecondaryIconScale;
        ring->mItemSlotParam2[index] =
            ring->mpItemBuf[index][0]->height / 48.0f * scale * kSecondaryIconScale;
    }
    load_ice_leaf_wheel_texture(ring);
    load_shield_surf_wheel_textures(ring);
    load_sit_wheel_texture(ring);
    load_lens_of_truth_wheel_texture(ring);
    load_goron_mask_wheel_texture(ring);
    load_giant_mask_wheel_texture(ring);
    load_armos_hammer_wheel_texture(ring);
    load_tunic_wheel_textures(ring);

    for (int button = 0; button < 2; ++button) {
        const u8 assignedSlot = g_secondarySelectedItems[button];
        ring->setSelectItem(button, secondaryDisplayItemForSlot(assignedSlot));
        if (button == 0) {
            ring->mXButtonSlot = dItemNo_NONE_e;
        } else {
            ring->mYButtonSlot = dItemNo_NONE_e;
        }
        for (u8 index = 0; index < kSecondaryWheelSlotCount; ++index) {
            if (g_secondaryWheelSlotIds[index] == assignedSlot) {
                if (button == 0) {
                    ring->mXButtonSlot = index;
                } else {
                    ring->mYButtonSlot = index;
                }
                break;
            }
        }
        if (secondaryTunicAssigned(button)) {
            for (auto* icon : ring->mpSelectItemTex[button]) {
                tint_tunic_icon(icon);
            }
        }
    }
    if (zItemSlotEnabled() && secondaryItemAssigned(kZItemSelectIndex)) {
        const u8 assignedSlot = g_secondarySelectedItems[kZItemSelectIndex];
        ring->field_0x6b3 = kZItemSelectIndex;
        ring->field_0x6b4[kZItemSelectIndex] = dItemNo_NONE_e;
        ring->field_0x6b8[kZItemSelectIndex] = dItemNo_NONE_e;
        ring->field_0x674[kZItemSelectIndex] = 0;
        for (u8 index = 0; index < kSecondaryWheelSlotCount; ++index) {
            if (g_secondaryWheelSlotIds[index] == assignedSlot) {
                ring->field_0x6ac = index;
                ring->field_0x6b4[kZItemSelectIndex] =
                    secondaryDisplayItemForSlot(assignedSlot);
                ring->setSelectItem(
                    kZItemSelectIndex, secondaryDisplayItemForSlot(assignedSlot));
                break;
            }
        }
    }
    ring->mpDrawCursor->setPos(
        g_secondaryWheelSlotPosX[0], g_secondaryWheelSlotPosY[0]);
    ring->mpDrawCursor->setParam(
        kSecondaryCursorScale, kSecondaryCursorScale, 0.1f, 0.6f, 0.5f);
    tint_secondary_ring_icons(ring);
    record_secondary_ring_pose(ring);
}

void set_secondary_ring_name(dMenu_Ring_c* ring) {
    if (ring->mpCenterScreen == nullptr || ring->mCurrentSlot >= kSecondaryWheelSlotCount) {
        return;
    }

    constexpr unsigned long long names[] = {
        MULTI_CHAR('item_n04'), MULTI_CHAR('item_n05'), MULTI_CHAR('item_n06'),
        MULTI_CHAR('item_n07'), MULTI_CHAR('fitem_n1'), MULTI_CHAR('fitem_n2'),
        MULTI_CHAR('fitem_n3'), MULTI_CHAR('fitem_n4')};
    for (const auto nameId : names) {
        if (auto* name = static_cast<J2DTextBox*>(ring->mpCenterScreen->search(nameId))) {
            name->setString(0x40,
                secondaryItemNameForSlot(g_secondaryWheelSlotIds[ring->mCurrentSlot]));
        }
    }
}

u8 closest_secondary_wheel_slot(s16 stickAngle) {
    u8 closestSlot = 0;
    int smallestDifference = 0x10000;
    for (u8 index = 0; index < kSecondaryWheelSlotCount; ++index) {
        const s16 difference = static_cast<s16>(
            stickAngle - g_secondaryWheelSlotAngles[index]);
        const int distance = std::abs(static_cast<int>(difference));
        if (distance < smallestDifference) {
            smallestDifference = distance;
            closestSlot = index;
        }
    }
    return closestSlot;
}

struct SecondaryRingPose {
    f32 centerX;
    f32 centerY;
    f32 alpha;
};

void record_secondary_ring_pose(dMenu_Ring_c* ring) {
#if TARGET_PC
    ring->captureRenderState();
#endif

    InterpMtx pose{};
    pose[0][0] = 1.0f;
    pose[1][1] = 1.0f;
    pose[2][2] = 1.0f;
    pose[0][3] = ring->mCenterPosX;
    pose[1][3] = ring->mCenterPosY;
    pose[2][3] = ring->mAlphaRate;
    if (svc_interp->record_mtx_keyed(mod_ctx, pose, ring) != MOD_OK &&
        !g_secondaryRingInterpolationErrorLogged)
    {
        g_secondaryRingInterpolationErrorLogged = true;
        svc_log->error(mod_ctx, "failed to record secondary wheel interpolation pose");
    }
}

SecondaryRingPose read_secondary_ring_pose(const dMenu_Ring_c* ring) {
    InterpMtx pose{};
    if (svc_interp->lookup_replacement_mtx(ring, pose)) {
        return {pose[0][3], pose[1][3], pose[2][3]};
    }
    return {ring->mCenterPosX, ring->mCenterPosY, ring->mAlphaRate};
}

void forget_secondary_ring_pose(const dMenu_Ring_c* ring) {
    if (ring != nullptr && svc_interp->forget_mtx(mod_ctx, ring) != MOD_OK) {
        svc_log->error(mod_ctx, "failed to release secondary wheel interpolation pose");
    }
}

void position_secondary_cursor(dMenu_Ring_c* ring);

bool is_ice_arrow_slot(const dMenu_Ring_c* ring) {
    if (ring == nullptr || ring->mCurrentSlot >= kSecondaryWheelSlotCount) {
        return false;
    }

    const u8 selected = g_secondaryWheelSlotIds[ring->mCurrentSlot];
    return selected >= kSecondarySlotBase &&
           selected < kSecondarySlotBase + MAX_ITEM_SLOTS &&
           g_secondarySlots[selected - kSecondarySlotBase] == kIceArrowItem;
}

bool can_use_ice_arrow_combo(const dMenu_Ring_c* ring) {
    if (ring == nullptr || ring->mPlayerIsWolf) {
        return false;
    }

    for (int button = 0; button < 2; ++button) {
        const u8 mixItem = dComIfGs_getMixItemIndex(button);
        if ((dComIfGs_getSelectItemIndex(button) == SLOT_4 &&
             mixItem == dItemNo_NONE_e) ||
            mixItem == SLOT_4)
        {
            return true;
        }
    }
    return false;
}

void set_ice_arrow_selected(daAlink_c* player, bool selected) {
    if (player == nullptr || g_iceArrowSelected == selected) {
        return;
    }

    g_iceArrowSelected = selected;
    if (player->mItemAcKeep.getActor() != nullptr) {
        player->deleteArrow();
        player->makeArrow();
        player->setBowReloadAnime();
    }
}

void begin_z_slot_assignment_animation(
    dMenu_Ring_c* ring, u8 sourceSlot, u8 savedSelection, u8 displayItem);
void clear_secondary_assignment(int button);
void clear_duplicate_z_assignment_for_item(
    dMenu_Ring_c* ring, int button, u8 slot, u8 item);

void move_secondary_ring(dMenu_Ring_c* ring) {
    if (ring->mStatus == dMenu_Ring_c::STATUS_WAIT) {
        if (ring->mWaitFrames > 0) {
            --ring->mWaitFrames;
        } else if (ring->mpStick != nullptr &&
                   ring->mpStick->getValueStick() >= 0.75f)
        {
            const u8 targetSlot =
                closest_secondary_wheel_slot(ring->mpStick->getAngleStick());
            if (targetSlot != ring->mCurrentSlot) {
                const s16 angleDelta = static_cast<s16>(
                    g_secondaryWheelSlotAngles[targetSlot] -
                    g_secondaryWheelSlotAngles[ring->mCurrentSlot]);
                ring->mCurrentSlot = angleDelta < 0
                    ? (ring->mCurrentSlot + 1) % kSecondaryWheelSlotCount
                    : (ring->mCurrentSlot + kSecondaryWheelSlotCount - 1) %
                        kSecondaryWheelSlotCount;
                ring->field_0x670 = g_secondaryWheelSlotAngles[ring->mCurrentSlot];
                ring->mWaitFrames =
                    std::max<s16>(g_ringHIO.mCursorChangeWaitFrames, 0);
            }
        }
    }

    if (ring->mCurrentSlot < kSecondaryWheelSlotCount) {
        const u8 selected = g_secondaryWheelSlotIds[ring->mCurrentSlot];
        const bool iceArrowSlot = is_ice_arrow_slot(ring);
        auto* player = daAlink_getAlinkActorClass();
        if (ring->mStatus == dMenu_Ring_c::STATUS_WAIT && iceArrowSlot &&
            mDoCPd_c::getTrigR(PAD_1) != 0 &&
            can_use_ice_arrow_combo(ring) && player != nullptr &&
            !player->checkCanoeSlider())
        {
            const bool selectingIceArrow = !g_iceArrowSelected;
            set_ice_arrow_selected(player, selectingIceArrow);
            Z2GetAudioMgr()->seStart(
                selectingIceArrow ? Z2SE_SY_ITEM_COMBINE_ON : Z2SE_SY_ITEM_COMBINE_OFF,
                NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        }
        if (!iceArrowSlot && mDoCPd_c::getTrigX(PAD_1) != 0) {
            g_secondarySelectedItems[0] = selected;
            ring->mXButtonSlot = ring->mCurrentSlot;
            ring->setSelectItem(0, secondaryDisplayItemForSlot(selected));
            const u8 selectedItem = secondaryItemIdForSlot(selected);
            clear_duplicate_z_assignment_for_item(
                ring, 0, selected, selectedItem);
            if (selectedItem == kGoronMaskItem ||
                selectedItem == kGiantMaskItem)
            {
                if (selectedItem == kGoronMaskItem) {
                    g_goronMaskActive = false;
                } else {
                    g_giantMaskActive = false;
                }
                g_goronMaskInputBlockFrames = 2;
                g_secondaryItemInputHandled = true;
            }
        }
        if (!iceArrowSlot && mDoCPd_c::getTrigY(PAD_1) != 0) {
            g_secondarySelectedItems[1] = selected;
            ring->mYButtonSlot = ring->mCurrentSlot;
            ring->setSelectItem(1, secondaryDisplayItemForSlot(selected));
            const u8 selectedItem = secondaryItemIdForSlot(selected);
            clear_duplicate_z_assignment_for_item(
                ring, 1, selected, selectedItem);
            if (selectedItem == kGoronMaskItem ||
                selectedItem == kGiantMaskItem)
            {
                if (selectedItem == kGoronMaskItem) {
                    g_goronMaskActive = false;
                } else {
                    g_giantMaskActive = false;
                }
                g_goronMaskInputBlockFrames = 2;
                g_secondaryItemInputHandled = true;
            }
        }
        if (!iceArrowSlot && zItemSlotEnabled() &&
            mDoCPd_c::getTrigZ(PAD_1) != 0)
        {
            g_secondarySelectedItems[kZItemSelectIndex] = selected;
            for (int button = 0; button < kZItemSelectIndex; ++button) {
                if (g_secondarySelectedItems[button] != selected) {
                    continue;
                }
                clear_secondary_assignment(button);
                ring->setSelectItem(button, dItemNo_NONE_e);
                if (button == 0) {
                    ring->mXButtonSlot = dItemNo_NONE_e;
                } else {
                    ring->mYButtonSlot = dItemNo_NONE_e;
                }
            }
            begin_z_slot_assignment_animation(
                ring, ring->mCurrentSlot,
                secondaryDisplayItemForSlot(selected),
                secondaryDisplayItemForSlot(selected));
        }
        for (int button = 0; button < 2; ++button) {
            if (secondaryTunicAssigned(button)) {
                for (auto* icon : ring->mpSelectItemTex[button]) {
                    tint_tunic_icon(icon);
                }
            }
        }
        if (!secondaryTunicAssigned(0) && !secondaryTunicAssigned(1) &&
            !secondaryTunicAssigned(kZItemSelectIndex))
        {
            set_goron_tunic_active(false);
            g_goronLavaContactFrames = 0;
        }
        if (!secondaryHoverBootsAssigned(0) && !secondaryHoverBootsAssigned(1) &&
            !secondaryHoverBootsAssigned(kZItemSelectIndex))
        {
            clear_hover_water_ground_plane();
            g_hoverBootsEquipped = false;
            g_hoverGroundHeightValid = false;
            g_hoverLandingSequenceActive = false;
            g_hoverAtSurface = false;
            g_hoverLandingFrames = 0;
            g_hoverRippleEmitter = 0;
            stop_hover_magic_effects();
        }
    }
    position_secondary_cursor(ring);
    set_secondary_ring_name(ring);
    ring->setMixMessage();
    record_secondary_ring_pose(ring);
}

void position_secondary_cursor(dMenu_Ring_c* ring) {
    if (ring->mCurrentSlot >= kSecondaryWheelSlotCount) {
        return;
    }

    const int index = ring->mCurrentSlot;
    ring->mpDrawCursor->setPos(g_secondaryWheelSlotPosX[index] + ring->mCenterPosX,
        g_secondaryWheelSlotPosY[index] + ring->mCenterPosY);
    ring->mpDrawCursor->setParam(
        kSecondaryCursorScale, kSecondaryCursorScale, 0.1f, 0.6f, 0.5f);
}

HookAction open_secondary_ring(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }

    if (ring->mOpenCloseFrames == 0) {
        dMeter2Info_set2DVibrationM();
    }
    ++ring->mOpenCloseFrames;
    ring->mAlphaRate = static_cast<f32>(ring->mOpenCloseFrames) / g_ringHIO.mOpenFrames;
    if (ring->mRingOrigin == 0) {
        ring->mCenterPosX = 0.0f;
        ring->mCenterPosY = (1.0f - ring->mAlphaRate) * FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 2) {
        ring->mCenterPosX = 0.0f;
        ring->mCenterPosY = (1.0f - ring->mAlphaRate) * -FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 3) {
        ring->mCenterPosX = (1.0f - ring->mAlphaRate) * FB_WIDTH_BASE;
        ring->mCenterPosY = 0.0f;
    } else {
        ring->mCenterPosX = (1.0f - ring->mAlphaRate) * -FB_WIDTH_BASE;
        ring->mCenterPosY = 0.0f;
    }
    const bool opened = ring->mOpenCloseFrames >= g_ringHIO.mOpenFrames;
    if (opened) {
        ring->mOpenCloseFrames = g_ringHIO.mCloseFrames;
        ring->mAlphaRate = 1.0f;
        ring->mCenterPosX = 0.0f;
        ring->mCenterPosY = 0.0f;
    }
    position_secondary_cursor(ring);
    record_secondary_ring_pose(ring);
    *static_cast<bool*>(retval) = opened;
    return HOOK_SKIP_ORIGINAL;
}

HookAction close_secondary_ring(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }

    if (ring->field_0x674[0] != 0 || ring->field_0x674[1] != 0 ||
        ring->field_0x674[2] != 0 || ring->field_0x674[3] != 0)
    {
        record_secondary_ring_pose(ring);
        *static_cast<bool*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }

    --ring->mOpenCloseFrames;
    ring->mAlphaRate = static_cast<f32>(ring->mOpenCloseFrames) / g_ringHIO.mCloseFrames;
    const bool closed = ring->mOpenCloseFrames <= 0;
    if (closed) {
        ring->mOpenCloseFrames = 0;
        ring->mAlphaRate = 0.0f;
    }
    if (ring->mRingOrigin == 0) {
        ring->mCenterPosX = 0.0f;
        ring->mCenterPosY = (1.0f - ring->mAlphaRate) * -FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 2) {
        ring->mCenterPosX = 0.0f;
        ring->mCenterPosY = (1.0f - ring->mAlphaRate) * FB_HEIGHT_BASE;
    } else if (ring->mRingOrigin == 3) {
        ring->mCenterPosX = (1.0f - ring->mAlphaRate) * -FB_WIDTH_BASE;
        ring->mCenterPosY = 0.0f;
    } else {
        ring->mCenterPosX = (1.0f - ring->mAlphaRate) * FB_WIDTH_BASE;
        ring->mCenterPosY = 0.0f;
    }
    position_secondary_cursor(ring);
    record_secondary_ring_pose(ring);
    *static_cast<bool*>(retval) = closed;
    return HOOK_SKIP_ORIGINAL;
}

void draw_secondary_ring(dMenu_Ring_c* ring) {
    const SecondaryRingPose pose = read_secondary_ring_pose(ring);
    J2DGrafContext* grafPort = dComIfGp_getCurrentGrafPort();
    grafPort->setup2D();
    if (ring->mRingPosX != g_ringHIO.mRingPosX ||
        ring->mRingPosY != g_ringHIO.mRingPosY)
    {
        ring->mRingPosX = g_ringHIO.mRingPosX;
        ring->mRingPosY = g_ringHIO.mRingPosY;
        ring->mpCircle->paneTrans(ring->mRingPosX, ring->mRingPosY);
    }
    if (ring->mRingScaleH != g_ringHIO.mRingScaleH ||
        ring->mRingScaleV != g_ringHIO.mRingScaleV)
    {
        ring->mRingScaleH = g_ringHIO.mRingScaleH;
        ring->mRingScaleV = g_ringHIO.mRingScaleV;
        ring->mpCircle->scale(ring->mRingScaleH, ring->mRingScaleV);
    }
    ring->mpSpotParent->setAlphaRate(pose.alpha * g_ringHIO.mOverlayAlpha);
    ring->mpSpotScreen->draw(0.0f, 0.0f, grafPort);
    ring->mpCenterParent->setAlphaRate(pose.alpha);
    ring->mpCenterScreen->draw(pose.centerX, pose.centerY, grafPort);
    ring->mpMessageParent->setAlphaRate(pose.alpha);
    ring->mpScreen->draw(pose.centerX, pose.centerY, grafPort);
    if (ring->mPikariFlashingSpeed > 0.0f) {
        CPaneMgr paneMgr;
        auto* buttonPane = ring->mpScreen->search(MULTI_CHAR('gr_btn'));
        if (buttonPane != nullptr) {
            const Vec pos = paneMgr.getGlobalVtxCenter(buttonPane, true, 0);
            dMeter2Info_getMeterClass()->getMeterDrawPtr()->drawPikari(
                pos.x + pose.centerX, pos.y + pose.centerY,
                &ring->mPikariFlashingSpeed, g_ringHIO.mPikariScale,
                g_ringHIO.mPikariFrontOuter, g_ringHIO.mPikariFrontInner,
                g_ringHIO.mPikariBackOuter, g_ringHIO.mPikariBackInner,
                g_ringHIO.mPikariAnimSpeed, 2);
        }
    }

    std::array<std::array<J2DPicture*, 3>, MAX_ITEM_SLOTS> savedItemTextures{};
    for (int index = 0; index < MAX_ITEM_SLOTS; ++index) {
        for (int texture = 0; texture < 3; ++texture) {
            savedItemTextures[index][texture] = ring->mpItemTex[index][texture];
            ring->mpItemTex[index][texture] = nullptr;
        }
    }

    ring->drawItem();
    const u8 savedCurrentSlot = ring->mCurrentSlot;
    const f32 savedLastSlotX = ring->mItemSlotPosX[MAX_ITEM_SLOTS - 1];
    const f32 savedLastSlotY = ring->mItemSlotPosY[MAX_ITEM_SLOTS - 1];
    if (ring->mCurrentSlot >= MAX_ITEM_SLOTS) {
        ring->mCurrentSlot = MAX_ITEM_SLOTS - 1;
        ring->mItemSlotPosX[MAX_ITEM_SLOTS - 1] =
            g_secondaryWheelSlotPosX[savedCurrentSlot];
        ring->mItemSlotPosY[MAX_ITEM_SLOTS - 1] =
            g_secondaryWheelSlotPosY[savedCurrentSlot];
    }
    ring->drawItem2();
    ring->mCurrentSlot = savedCurrentSlot;
    ring->mItemSlotPosX[MAX_ITEM_SLOTS - 1] = savedLastSlotX;
    ring->mItemSlotPosY[MAX_ITEM_SLOTS - 1] = savedLastSlotY;
    for (int index = 0; index < MAX_ITEM_SLOTS; ++index) {
        for (int texture = 0; texture < 3; ++texture) {
            ring->mpItemTex[index][texture] = savedItemTextures[index][texture];
        }
    }

    for (u8 index = 0; index < kSecondaryWheelSlotCount; ++index) {
        const f32 x = g_secondaryWheelSlotPosX[index] - 24.0f + pose.centerX;
        const f32 y = g_secondaryWheelSlotPosY[index] - 24.0f + pose.centerY;
        if (index == MAX_ITEM_SLOTS) {
            if (g_iceLeafWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_iceLeafWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_iceLeafWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
            }
            continue;
        }
        if (index == MAX_ITEM_SLOTS + 1) {
            if (g_shieldSurfWheelItemBuf[0] != nullptr) {
                const u8 shieldItem = shield_surf_display_item();
                const f32 scale =
                    dItem_data::getTexScale(shieldItem) / 100.0f * kSecondaryIconScale;
                const f32 width = g_shieldSurfWheelItemBuf[0]->width / 48.0f * scale * 48.0f;
                const f32 height = g_shieldSurfWheelItemBuf[0]->height / 48.0f * scale * 48.0f;
                for (auto* picture : g_shieldSurfWheelItemTex) {
                    if (picture == nullptr) {
                        continue;
                    }
                    picture->setAlpha(pose.alpha * 255.0f);
                    picture->draw(x + (48.0f - width) * 0.5f,
                        y + (48.0f - height) * 0.5f, width, height, 0, 0, 0);
                }
            }
            continue;
        }
        if (index == MAX_ITEM_SLOTS + 2) {
            if (g_sitWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_sitWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_sitWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
                continue;
            }
        }
        if (index == MAX_ITEM_SLOTS + 3) {
            if (g_tunicWheelItemBuf[0] == nullptr) {
                continue;
            }
            const f32 scale =
                dItem_data::getTexScale(kGoronTunicDisplayItem) / 100.0f * kSecondaryIconScale;
            const f32 width = g_tunicWheelItemBuf[0]->width / 48.0f * scale * 48.0f;
            const f32 height = g_tunicWheelItemBuf[0]->height / 48.0f * scale * 48.0f;
            for (auto* picture : g_tunicWheelItemTex) {
                if (picture == nullptr) {
                    continue;
                }
                picture->setAlpha(pose.alpha * 255.0f);
                picture->draw(x + (48.0f - width) * 0.5f,
                    y + (48.0f - height) * 0.5f, width, height, 0, 0, 0);
            }
            continue;
        }

        if (secondaryItemIdForSlot(g_secondaryWheelSlotIds[index]) ==
            kLensOfTruthItem)
        {
            if (g_lensOfTruthWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_lensOfTruthWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_lensOfTruthWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
            }
            continue;
        }

        if (secondaryItemIdForSlot(g_secondaryWheelSlotIds[index]) ==
            kGoronMaskItem)
        {
            if (g_goronMaskWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_goronMaskWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_goronMaskWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
            }
            continue;
        }

        if (secondaryItemIdForSlot(g_secondaryWheelSlotIds[index]) ==
            kGiantMaskItem)
        {
            if (g_giantMaskWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_giantMaskWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_giantMaskWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
            }
            continue;
        }

        if (secondaryItemIdForSlot(g_secondaryWheelSlotIds[index]) ==
            kArmosHammerItem)
        {
            if (g_armosHammerWheelItemTex != nullptr) {
                constexpr f32 iconSize = 36.0f;
                g_armosHammerWheelItemTex->setAlpha(pose.alpha * 255.0f);
                g_armosHammerWheelItemTex->draw(
                    x + (48.0f - iconSize) * 0.5f,
                    y + (48.0f - iconSize) * 0.5f,
                    iconSize, iconSize, 0, 0, 0);
            }
            continue;
        }

        for (int texture = 0; texture < 3; ++texture) {
            auto* picture = ring->mpItemTex[index][texture];
            if (picture == nullptr) {
                continue;
            }
            picture->setAlpha(pose.alpha * 255.0f);
            const f32 width = ring->mItemSlotParam1[index] * 48.0f;
            const f32 height = ring->mItemSlotParam2[index] * 48.0f;
            picture->draw(x + (48.0f - width) * 0.5f,
                y + (48.0f - height) * 0.5f, width, height, 0, 0, 0);
        }
    }

    constexpr unsigned long long buttonPanes[] = {
        MULTI_CHAR('x_btn_n'), MULTI_CHAR('y_btn_n')};
    for (int button = 0; button < 2; ++button) {
        if (!secondaryItemAssigned(button)) {
            continue;
        }
        auto* pane = ring->mpScreen->search(buttonPanes[button]);
        if (pane == nullptr) {
            continue;
        }
        CPaneMgr paneMgr;
        const Vec center = paneMgr.getGlobalVtxCenter(pane, true, 0);
        for (int texture = 0; texture < ring->field_0x686[button]; ++texture) {
            auto* picture = ring->mpSelectItemTex[button][texture];
            if (picture == nullptr) {
                continue;
            }
            picture->setAlpha(pose.alpha * 255.0f);
            const f32 width = ring->field_0x548[button] * 48.0f;
            const f32 height = ring->field_0x558[button] * 48.0f;
            picture->draw(center.x - width * 0.5f + pose.centerX,
                center.y - height * 0.5f + pose.centerY,
                width, height, 0, 0, 0);
        }
    }

    if (ring->field_0x674[kZItemSelectIndex] != 0) {
        ring->drawSelectItem();
    }

    if (ring->mCurrentSlot < kSecondaryWheelSlotCount) {
        ring->mpDrawCursor->setPos(
            g_secondaryWheelSlotPosX[ring->mCurrentSlot] + pose.centerX,
            g_secondaryWheelSlotPosY[ring->mCurrentSlot] + pose.centerY);
    }
    ring->mpDrawCursor->draw();
}

HookAction suppress_secondary_explain(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (g_openingSecondaryRing || isSecondaryRing(ring)) {
        *static_cast<u8*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void name_secondary_item(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring) || ring->mCurrentSlot >= kSecondaryWheelSlotCount ||
        ring->mpCenterScreen == nullptr)
    {
        return;
    }

    constexpr unsigned long long names[] = {
        MULTI_CHAR('item_n04'), MULTI_CHAR('item_n05'), MULTI_CHAR('item_n06'),
        MULTI_CHAR('item_n07'), MULTI_CHAR('fitem_n1'), MULTI_CHAR('fitem_n2'),
        MULTI_CHAR('fitem_n3'), MULTI_CHAR('fitem_n4')};
    const char* itemName = secondaryItemNameForSlot(
        g_secondaryWheelSlotIds[ring->mCurrentSlot]);
    for (const auto nameId : names) {
        if (auto* name = static_cast<J2DTextBox*>(ring->mpCenterScreen->search(nameId))) {
            name->setString(0x40, itemName);
        }
    }
}

HookAction begin_key_wait(ModContext*, void*, void*, void*) {
    ++g_keyWaitDepth;
    return HOOK_CONTINUE;
}

void end_key_wait(ModContext*, void*, void*, void*) {
    if (g_keyWaitDepth > 0) {
        --g_keyWaitDepth;
    }
}

HookAction begin_map_input(ModContext*, void*, void*, void*) {
    ++g_mapInputDepth;
    return HOOK_CONTINUE;
}

void end_map_input(ModContext*, void*, void*, void*) {
    if (g_mapInputDepth > 0) {
        --g_mapInputDepth;
    }
}

HookAction remap_down_trigger(ModContext*, void*, void* retval, void*) {
    if (mapOpensDown() &&
        (g_keyWaitDepth > 0 || g_secondaryRing != nullptr))
    {
        *static_cast<BOOL*>(retval) = mDoCPd_c::getTrigLeft(PAD_1) != 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction remap_left_trigger(ModContext*, void*, void* retval, void*) {
    if (z_slot_midna_left_triggered() && g_mapInputDepth > 0) {
        *static_cast<BOOL*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    if (mapOpensDown() && g_keyWaitDepth > 0) {
        *static_cast<BOOL*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    if (mapOpensDown() && g_mapInputDepth > 0) {
        *static_cast<BOOL*>(retval) = mDoCPd_c::getTrigDown(PAD_1) != 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction prepare_secondary_wheel(ModContext*, void* args, void*, void*) {
    g_openingSecondaryRing = false;
    if (!secondaryWheelBindingEnabled()) {
        return HOOK_CONTINUE;
    }

    const bool secondaryPressed =
        wheelOpensDown() ? mDoCPd_c::getTrigDown(PAD_1) != 0 :
                           mDoCPd_c::getTrigLeft(PAD_1) != 0;
    const bool primaryPressed = mDoCPd_c::getTrigUp(PAD_1) != 0;
    if (g_keyWaitDepth > 0 && secondaryPressed && !primaryPressed) {
        g_openingSecondaryRing = true;
        mods::arg_ref<u8>(args, 1) = mapOpensDown() ? 1 : 2;
    }

    return HOOK_CONTINUE;
}

void finish_secondary_wheel(ModContext*, void* args, void*, void*) {
    auto* window = mods::arg<dMw_c*>(args, 0);
    if (g_openingSecondaryRing) {
        g_secondaryRing = window->mpMenuRing;
        setup_secondary_ring(g_secondaryRing);
    } else {
        g_primaryRingOpen = true;
        g_pendingPrimaryItemSync = true;
    }
    g_openingSecondaryRing = false;
}

HookAction prepare_ring_delete(ModContext*, void* args, void*, void*) {
    auto* window = mods::arg<dMw_c*>(args, 0);
    g_deletingSecondaryRing = isSecondaryRing(window->mpMenuRing);
    if (g_deletingSecondaryRing) {
        forget_secondary_ring_pose(window->mpMenuRing);
        clear_ice_leaf_wheel_texture();
        clear_shield_surf_wheel_textures();
        clear_sit_wheel_texture();
        clear_lens_of_truth_wheel_texture();
        clear_goron_mask_wheel_texture();
        clear_giant_mask_wheel_texture();
        clear_armos_hammer_wheel_texture();
        clear_tunic_wheel_textures();
    }
    return HOOK_CONTINUE;
}

void clear_secondary_wheel(ModContext*, void*, void*, void*) {
    if (g_deletingSecondaryRing) {
        g_secondaryRing = nullptr;
        g_deletingSecondaryRing = false;
    } else if (g_pendingPrimaryItemSync) {
        dComIfGp_setSelectItem(0);
        dComIfGp_setSelectItem(1);
        g_pendingPrimaryItemSync = false;
        g_primaryRingOpen = false;
    }
}

HookAction handle_secondary_ring_move_end(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }
    if (ring->mStatus != dMenu_Ring_c::STATUS_WAIT ||
        ring->mOldStatus == dMenu_Ring_c::STATUS_EXPLAIN_FORCE ||
        ring->mOldStatus == dMenu_Ring_c::STATUS_EXPLAIN)
    {
        *static_cast<bool*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }

    const bool closing = dMw_UP_TRIGGER() || dMw_DOWN_TRIGGER() || dMw_B_TRIGGER() ||
                         dMeter2Info_getWarpStatus() == 2 ||
                         dMeter2Info_getWarpStatus() == 1 ||
                         dMeter2Info_isTouchKeyCheck(0xe);
    if (closing) {
        if (mDoCPd_c::getTrigLeft(PAD_1) != 0) {
            ring->mRingOrigin = 1;
        } else if (mDoCPd_c::getTrigRight(PAD_1) != 0) {
            ring->mRingOrigin = 3;
        } else if (dMw_DOWN_TRIGGER()) {
            ring->mRingOrigin = 2;
        } else {
            ring->mRingOrigin = 0;
        }
        Z2GetAudioMgr()->seStart(
            Z2SE_ITEM_RING_OUT, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dMeter2Info_set2DVibrationM();
    }
    *static_cast<bool*>(retval) = closing;
    return HOOK_SKIP_ORIGINAL;
}

u8 z_slot_gale_bomb_base_slot();

HookAction show_ice_arrow_combo_on_prompt(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (ring != nullptr && !isSecondaryRing(ring) && retval != nullptr &&
        zItemSlotEnabled() && !secondaryItemAssigned(kZItemSelectIndex) &&
        !ring->mPlayerIsWolf && ring->mCurrentSlot < MAX_ITEM_SLOTS &&
        ring->mItemSlots[ring->mCurrentSlot] < MAX_ITEM_SLOTS)
    {
        const u8 cursorItem =
            dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false);
        const u8 zSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
        const u8 zMixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
        const bool bowComboItem =
            cursorItem == dItemNo_HAWK_EYE_e ||
            cursorItem == dItemNo_NORMAL_BOMB_e ||
            cursorItem == dItemNo_WATER_BOMB_e ||
            cursorItem == dItemNo_POKE_BOMB_e;
        const u8 galeSlot = z_slot_gale_bomb_base_slot();
        const bool galeBombComboItem =
            cursorItem == dItemNo_NORMAL_BOMB_e &&
            galeSlot != dItemNo_NONE_e &&
            galeSlot != ring->mItemSlots[ring->mCurrentSlot] &&
            !((zSlot == galeSlot &&
               zMixSlot == ring->mItemSlots[ring->mCurrentSlot]) ||
              (zSlot == ring->mItemSlots[ring->mCurrentSlot] &&
               zMixSlot == galeSlot));
        if (bowComboItem &&
            ((zSlot == SLOT_4 && zMixSlot != ring->mItemSlots[ring->mCurrentSlot]) ||
             (zMixSlot == SLOT_4 && zSlot != ring->mItemSlots[ring->mCurrentSlot])))
        {
            *static_cast<bool*>(retval) = true;
            return HOOK_SKIP_ORIGINAL;
        }
        if (galeBombComboItem) {
            *static_cast<bool*>(retval) = true;
            return HOOK_SKIP_ORIGINAL;
        }
    }
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }

    *static_cast<bool*>(retval) =
        ring->mStatus == dMenu_Ring_c::STATUS_WAIT && is_ice_arrow_slot(ring) &&
        !g_iceArrowSelected && can_use_ice_arrow_combo(ring);
    return HOOK_SKIP_ORIGINAL;
}

HookAction show_ice_arrow_combo_off_prompt(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (ring != nullptr && !isSecondaryRing(ring) && retval != nullptr &&
        zItemSlotEnabled() && !secondaryItemAssigned(kZItemSelectIndex) &&
        !ring->mPlayerIsWolf && ring->mCurrentSlot < MAX_ITEM_SLOTS &&
        ring->mItemSlots[ring->mCurrentSlot] < MAX_ITEM_SLOTS)
    {
        const u8 cursorSlot = ring->mItemSlots[ring->mCurrentSlot];
        const u8 cursorItem = dComIfGs_getItem(cursorSlot, false);
        const u8 zSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
        const u8 zMixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
        const bool bowComboItem =
            cursorItem == dItemNo_HAWK_EYE_e ||
            cursorItem == dItemNo_NORMAL_BOMB_e ||
            cursorItem == dItemNo_WATER_BOMB_e ||
            cursorItem == dItemNo_POKE_BOMB_e;
        const u8 galeSlot = z_slot_gale_bomb_base_slot();
        const bool galeBombComboItem =
            gale_bomb_combo_selected(kZItemSelectIndex) &&
            cursorItem == dItemNo_NORMAL_BOMB_e &&
            ((zSlot == cursorSlot && zMixSlot == galeSlot) ||
             (zSlot == galeSlot && zMixSlot == cursorSlot));
        if (bowComboItem &&
            ((zSlot == cursorSlot && zMixSlot == SLOT_4) ||
             (zSlot == SLOT_4 && zMixSlot == cursorSlot)))
        {
            *static_cast<bool*>(retval) = true;
            return HOOK_SKIP_ORIGINAL;
        }
        if (galeBombComboItem) {
            *static_cast<bool*>(retval) = true;
            return HOOK_SKIP_ORIGINAL;
        }
    }
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }

    *static_cast<bool*>(retval) =
        ring->mStatus == dMenu_Ring_c::STATUS_WAIT && is_ice_arrow_slot(ring) &&
        g_iceArrowSelected && can_use_ice_arrow_combo(ring);
    return HOOK_SKIP_ORIGINAL;
}

bool is_bow_combo_item(u8 item) {
    return item == dItemNo_HAWK_EYE_e ||
           item == dItemNo_NORMAL_BOMB_e ||
           item == dItemNo_WATER_BOMB_e ||
           item == dItemNo_POKE_BOMB_e;
}

u8 ring_cursor_for_inventory_slot(dMenu_Ring_c* ring, u8 inventorySlot);
void sync_primary_ring_item_selection(dMenu_Ring_c* ring);

u8 z_slot_gale_bomb_base_slot() {
    const u8 zSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
    const u8 zMixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
    if (zSlot != dItemNo_NONE_e &&
        dComIfGs_getItem(zSlot, false) == dItemNo_BOOMERANG_e)
    {
        return zSlot;
    }
    if (zMixSlot != dItemNo_NONE_e &&
        dComIfGs_getItem(zMixSlot, false) == dItemNo_BOOMERANG_e)
    {
        return zMixSlot;
    }
    if ((zSlot == dItemNo_NONE_e || zSlot == SLOT_4) &&
        dComIfGs_getItem(SLOT_4, false) == dItemNo_BOOMERANG_e)
    {
        return SLOT_4;
    }
    return dItemNo_NONE_e;
}

bool toggle_z_gale_bomb_combo(
    dMenu_Ring_c* ring, u8 cursorSlot, u8 cursorItem)
{
    if (ring == nullptr || !zItemSlotEnabled() ||
        secondaryItemAssigned(kZItemSelectIndex) ||
        cursorItem != dItemNo_NORMAL_BOMB_e)
    {
        return false;
    }

    const u8 zSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
    const u8 zMixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
    const u8 galeSlot = z_slot_gale_bomb_base_slot();
    if (galeSlot == dItemNo_NONE_e || galeSlot == cursorSlot) {
        return false;
    }

    const bool turnOff =
        (zSlot == galeSlot && zMixSlot == cursorSlot) ||
        (zSlot == cursorSlot && zMixSlot == galeSlot);
    const bool canTurnOn =
        (zSlot == galeSlot && zMixSlot != cursorSlot) ||
        (zSlot == dItemNo_NONE_e && galeSlot == SLOT_4);
    if (!turnOff && !canTurnOn) {
        return false;
    }

    for (int button = 0; button <= kZItemSelectIndex; ++button) {
        ring->setSelectItemForce(button);
    }

    if (turnOff) {
        Z2GetAudioMgr()->seStart(
            Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dComIfGs_setSelectItemIndex(kZItemSelectIndex, galeSlot);
        dComIfGs_setMixItemIndex(kZItemSelectIndex, dItemNo_NONE_e);
        ring->field_0x6cb = cursorSlot;
        ring->field_0x6cd = kZItemSelectIndex;
    } else {
        Z2GetAudioMgr()->seStart(
            Z2SE_SY_ITEM_COMBINE_ON, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dComIfGs_setSelectItemIndex(kZItemSelectIndex, galeSlot);
        dComIfGs_setMixItemIndex(kZItemSelectIndex, cursorSlot);
        ring->field_0x6cd = dItemNo_NONE_e;

        for (int button = 0; button < kZItemSelectIndex; ++button) {
            if (dComIfGs_getSelectItemIndex(button) == cursorSlot) {
                dComIfGs_setSelectItemIndex(button, dItemNo_NONE_e);
                dComIfGs_setMixItemIndex(button, dItemNo_NONE_e);
            } else if (dComIfGs_getMixItemIndex(button) == cursorSlot) {
                dComIfGs_setMixItemIndex(button, dItemNo_NONE_e);
                ring->setSelectItem(button, selected_item_for_button(button));
            }
        }
    }

    sync_primary_ring_item_selection(ring);
    ring->field_0x6b3 = kZItemSelectIndex;
    ring->field_0x674[kZItemSelectIndex] = 1;
    ring->setJumpItem(false);
    ring->setSelectItem(
        kZItemSelectIndex, selected_item_for_button(kZItemSelectIndex));

    const u8 animationInventorySlot =
        ring->field_0x6cd == kZItemSelectIndex
            ? ring->field_0x6cb
            : ring->field_0x6b4[kZItemSelectIndex];
    const u8 animationSlot =
        ring_cursor_for_inventory_slot(ring, animationInventorySlot);
    if (animationSlot != dItemNo_NONE_e) {
        ring->field_0x518[kZItemSelectIndex] = ring->mItemSlotPosX[animationSlot];
        ring->field_0x528[kZItemSelectIndex] = ring->mItemSlotPosY[animationSlot];
    }
    ring->field_0x538[kZItemSelectIndex] = g_ringHIO.mSelectItemScale;
    return true;
}

u8 ring_cursor_for_inventory_slot(dMenu_Ring_c* ring, u8 inventorySlot) {
    if (ring == nullptr || inventorySlot == dItemNo_NONE_e) {
        return dItemNo_NONE_e;
    }
    for (u8 cursor = 0; cursor < MAX_ITEM_SLOTS; ++cursor) {
        if (ring->mItemSlots[cursor] == inventorySlot) {
            return cursor;
        }
    }
    return dItemNo_NONE_e;
}

void sync_primary_ring_item_selection(dMenu_Ring_c* ring) {
    ring->mXButtonSlot =
        ring_cursor_for_inventory_slot(ring, dComIfGs_getSelectItemIndex(0));
    ring->mYButtonSlot =
        ring_cursor_for_inventory_slot(ring, dComIfGs_getSelectItemIndex(1));
    ring->field_0x6ac = ring_cursor_for_inventory_slot(
        ring, dComIfGs_getSelectItemIndex(kZItemSelectIndex));
    for (int button = 0; button <= kZItemSelectIndex; ++button) {
        ring->field_0x6b4[button] = dComIfGs_getSelectItemIndex(button);
        ring->field_0x6b8[button] = dComIfGs_getMixItemIndex(button);
    }
}

bool toggle_z_bow_combo(dMenu_Ring_c* ring, u8 cursorSlot, u8 cursorItem) {
    if (ring == nullptr || !zItemSlotEnabled() ||
        secondaryItemAssigned(kZItemSelectIndex) ||
        dComIfGs_getItem(SLOT_4, false) != dItemNo_BOW_e ||
        !is_bow_combo_item(cursorItem))
    {
        return false;
    }

    const u8 zSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
    const u8 zMixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
    const bool turnOff =
        (zSlot == cursorSlot && zMixSlot == SLOT_4) ||
        (zSlot == SLOT_4 && zMixSlot == cursorSlot);
    const bool canTurnOn =
        (zSlot == SLOT_4 && zMixSlot != cursorSlot) || zMixSlot == SLOT_4;
    if (!turnOff && !canTurnOn) {
        return false;
    }

    for (int button = 0; button <= kZItemSelectIndex; ++button) {
        ring->setSelectItemForce(button);
    }

    if (turnOff) {
        Z2GetAudioMgr()->seStart(
            Z2SE_SY_ITEM_COMBINE_OFF, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dComIfGs_setSelectItemIndex(kZItemSelectIndex, SLOT_4);
        dComIfGs_setMixItemIndex(kZItemSelectIndex, dItemNo_NONE_e);
        ring->field_0x6cb = zSlot == SLOT_4 ? zMixSlot : zSlot;
        ring->field_0x6cd = kZItemSelectIndex;
    } else {
        Z2GetAudioMgr()->seStart(
            Z2SE_SY_ITEM_COMBINE_ON, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dComIfGs_setSelectItemIndex(kZItemSelectIndex, cursorSlot);
        dComIfGs_setMixItemIndex(kZItemSelectIndex, SLOT_4);
        ring->field_0x6cd = dItemNo_NONE_e;

        for (int button = 0; button < kZItemSelectIndex; ++button) {
            if (dComIfGs_getSelectItemIndex(button) == cursorSlot) {
                dComIfGs_setSelectItemIndex(button, dItemNo_NONE_e);
                dComIfGs_setMixItemIndex(button, dItemNo_NONE_e);
            }
        }
    }

    sync_primary_ring_item_selection(ring);
    ring->field_0x6b3 = kZItemSelectIndex;
    ring->field_0x674[kZItemSelectIndex] = 1;
    ring->setJumpItem(false);
    ring->setSelectItem(kZItemSelectIndex, selected_item_for_button(kZItemSelectIndex));

    const u8 animationInventorySlot =
        ring->field_0x6cd == kZItemSelectIndex
            ? ring->field_0x6cb
            : ring->field_0x6b4[kZItemSelectIndex];
    const u8 animationSlot =
        ring_cursor_for_inventory_slot(ring, animationInventorySlot);
    if (animationSlot != dItemNo_NONE_e) {
        ring->field_0x518[kZItemSelectIndex] = ring->mItemSlotPosX[animationSlot];
        ring->field_0x528[kZItemSelectIndex] = ring->mItemSlotPosY[animationSlot];
    }
    ring->field_0x538[kZItemSelectIndex] = g_ringHIO.mSelectItemScale;
    return true;
}

HookAction handle_secondary_ring_move(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }
    move_secondary_ring(ring);
    return HOOK_SKIP_ORIGINAL;
}

HookAction handle_secondary_ring_draw(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }
    draw_secondary_ring(ring);
    return HOOK_SKIP_ORIGINAL;
}

const char* secondary_custom_icon_resource(SecondaryIconContext context);

DEFINE_HOOK(&dComIfGp_getSelectItem, ZSelectItemAccessorHook);
DEFINE_HOOK(&daAlink_c::checkItemSetButton, CheckItemSetButtonHook);

void resolve_z_selected_item(ModContext*, void* args, void* retval, void*) {
    const int button = mods::arg<int>(args, 0);
    if (!zItemSlotEnabled() || retval == nullptr) {
        return;
    }

   if (button == kZItemSelectIndex) {
        *static_cast<u8*>(retval) = selected_item_for_button(kZItemSelectIndex);
    }
}

void count_z_item_as_assigned(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    auto* result = static_cast<int*>(retval);
    const int itemNo = mods::arg<int>(args, 1);
    if (!zItemSlotEnabled() || alink == nullptr || result == nullptr ||
        *result != 2 || alink->checkWolf())
    {
        return;
    }

    const u8 zItem = selected_item_for_button(kZItemSelectIndex);
    if (zItem != dItemNo_NONE_e && alink->checkGroupItem(itemNo, zItem)) {
        // checkItemSetButton reserves 2 as "not assigned" and only scans X/Y.
        *result = 0;
    }
}

void replace_secondary_custom_icon(ModContext*, void* args, void*, void*) {
    if ((g_secondaryIconContext != SecondaryIconContext::IceLeaf &&
         g_secondaryIconContext != SecondaryIconContext::Sit &&
         g_secondaryIconContext != SecondaryIconContext::HoverBoots &&
         g_secondaryIconContext != SecondaryIconContext::DarkLink &&
         g_secondaryIconContext != SecondaryIconContext::IceArrow &&
         g_secondaryIconContext != SecondaryIconContext::LensOfTruth &&
         g_secondaryIconContext != SecondaryIconContext::TruthMask &&
         g_secondaryIconContext != SecondaryIconContext::GoronMask &&
         g_secondaryIconContext != SecondaryIconContext::GiantMask &&
         g_secondaryIconContext != SecondaryIconContext::ArmosHammer &&
         g_secondaryIconContext != SecondaryIconContext::DekuNut) ||
        mods::arg<u8>(args, 0) != dItemNo_WOOD_STICK_e)
    {
        return;
    }

    const char* resourceName =
        secondary_custom_icon_resource(g_secondaryIconContext);
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result = svc_resource->load(mod_ctx, resourceName, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load a custom secondary item icon");
        svc_resource->free(mod_ctx, &icon);
        return;
    }
    if (icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "custom secondary item icon exceeds the texture buffer");
        svc_resource->free(mod_ctx, &icon);
        return;
    }

    for (int index : {1, 3, 5}) {
        void* textureBuffer = mods::arg<void*>(args, index);
        if (textureBuffer != nullptr) {
            std::memcpy(textureBuffer, icon.data, icon.size);
        }
    }
    svc_resource->free(mod_ctx, &icon);
}

HookAction apply_hawk_grass_equip_icon(ModContext*, void* args, void*, void*) {
    if (g_secondaryIconContext == SecondaryIconContext::HawkGrass &&
        mods::arg<u8>(args, 0) == dItemNo_WOOD_STICK_e)
    {
        mods::arg_ref<int>(args, 9) = kHawkGrassIconTexture;
    }
    return HOOK_CONTINUE;
}

HookAction prepare_secondary_equip_icon(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    const int button = mods::arg<int>(args, 1);
    if (!isSecondaryRing(ring) || button < 0 || button >= kSecondaryAssignmentCount ||
        (button == kZItemSelectIndex && !zItemSlotEnabled()))
    {
        return HOOK_CONTINUE;
    }

    const u8 slot = g_secondarySelectedItems[button];
    g_secondaryIconContext = secondaryIconContextForSlot(slot);
    if (secondaryItemAssigned(button)) {
        mods::arg_ref<u8>(args, 2) = secondaryDisplayItemForSlot(slot);
    }
    return HOOK_CONTINUE;
}

void finish_secondary_equip_icon(ModContext*, void*, void*, void*) {
    g_secondaryIconContext = SecondaryIconContext::None;
}

const char* secondary_custom_icon_resource(SecondaryIconContext context) {
    switch (context) {
    case SecondaryIconContext::IceLeaf:
        return kIceLeafIconResource;
    case SecondaryIconContext::Sit:
        return kSitIconResource;
    case SecondaryIconContext::IceArrow:
        return kIceArrowIconResource;
    case SecondaryIconContext::LensOfTruth:
        return kLensOfTruthIconResource;
    case SecondaryIconContext::TruthMask:
        return kTruthMaskIconResource;
    case SecondaryIconContext::GoronMask:
        return kGoronMaskIconResource;
    case SecondaryIconContext::GiantMask:
        return kGiantMaskIconResource;
    case SecondaryIconContext::ArmosHammer:
        return kArmosHammerIconResource;
    case SecondaryIconContext::DekuNut:
        return kDekuNutIconResource;
    case SecondaryIconContext::DarkLink:
        return kDarkLinkIconResource;
    case SecondaryIconContext::HoverBoots:
        return kHoverBootsIconResource;
    default:
        return nullptr;
    }
}

HookAction prepare_hover_boots_hud_icon(ModContext*, void* args, void*, void*) {
    const int button = mods::arg<int>(args, 1);
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (button < 0 || button >= kSecondaryAssignmentCount ||
        (button == kZItemSelectIndex && !zItemSlotEnabled()) ||
        !secondaryItemAssigned(button) ||
        (alink != nullptr && alink->checkWolf()))
    {
        g_secondaryIconContext = SecondaryIconContext::None;
        return HOOK_CONTINUE;
    }

    const u8 slot = g_secondarySelectedItems[button];
    g_secondaryIconContext = secondaryIconContextForSlot(slot);
    if (button == kZItemSelectIndex) {
        mods::arg_ref<u8>(args, 2) = secondaryDisplayItemForSlot(slot);
        return HOOK_CONTINUE;
    }
    if (g_secondaryIconContext != SecondaryIconContext::Sit &&
        g_secondaryIconContext != SecondaryIconContext::IceLeaf &&
        g_secondaryIconContext != SecondaryIconContext::HoverBoots &&
        g_secondaryIconContext != SecondaryIconContext::DarkLink &&
        g_secondaryIconContext != SecondaryIconContext::IceArrow &&
        g_secondaryIconContext != SecondaryIconContext::LensOfTruth &&
        g_secondaryIconContext != SecondaryIconContext::TruthMask &&
        g_secondaryIconContext != SecondaryIconContext::GoronMask &&
        g_secondaryIconContext != SecondaryIconContext::GiantMask &&
        g_secondaryIconContext != SecondaryIconContext::ArmosHammer &&
        g_secondaryIconContext != SecondaryIconContext::DekuNut)
    {
        mods::arg_ref<u8>(args, 2) = secondaryDisplayItemForSlot(slot);
        return HOOK_CONTINUE;
    }

    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr || meter->mpItemXY[button] == nullptr ||
        meter->mpItemXYPane[button] == nullptr)
    {
        svc_log->error(mod_ctx, "failed to access the X/Y HUD panes for a custom item");
        return HOOK_SKIP_ORIGINAL;
    }

    const char* resourceName =
        secondary_custom_icon_resource(g_secondaryIconContext);
    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result = svc_resource->load(mod_ctx, resourceName, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load a custom X/Y HUD icon");
        svc_resource->free(mod_ctx, &icon);
        return HOOK_SKIP_ORIGINAL;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "custom X/Y HUD icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        return HOOK_SKIP_ORIGINAL;
    }

    const u8 textureIndex = meter->field_0x76c[button] == 0 ? 1 : 0;
    meter->field_0x76c[button] = textureIndex;
    ResTIMG* texture = meter->mpItemXYTex[button][textureIndex][0];
    if (texture == nullptr) {
        svc_log->error(mod_ctx, "failed to access the custom X/Y HUD texture buffer");
        svc_resource->free(mod_ctx, &icon);
        return HOOK_SKIP_ORIGINAL;
    }

    std::memcpy(texture, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    if (texture->width == 0 || texture->height == 0) {
        svc_log->error(mod_ctx, "Hover Boots X/Y HUD icon has invalid dimensions");
        return HOOK_SKIP_ORIGINAL;
    }
    DCStoreRangeNoSync(texture, kItemTextureBufferSize);

    auto* picture = static_cast<J2DPicture*>(meter->mpItemXY[button]->getPanePtr());
    if (picture == nullptr || picture->changeTexture(texture, 0) == nullptr) {
        svc_log->error(mod_ctx, "failed to apply the custom X/Y HUD icon");
        return HOOK_SKIP_ORIGINAL;
    }
    picture->setBlackWhite(
        JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
    meter->mpItemXYPane[button]->hide();

    const f32 customIconScale =
        g_secondaryIconContext == SecondaryIconContext::DarkLink ? 1.5f : 1.0f;
    const f32 scale = (g_drawHIO.mItemScaleAdjustON
                           ? g_drawHIO.mItemScalePercent / 100.0f
                           : 1.0f) * customIconScale;
    meter->field_0x6c4[button] =
        scale * ((texture->width * meter->mpItemXY[button]->getInitSizeX()) / 48.0f);
    meter->field_0x6d0[button] =
        scale * ((texture->height * meter->mpItemXY[button]->getInitSizeY()) / 48.0f);
    meter->field_0x6ac[button] =
        (meter->mpItemXY[button]->getInitSizeX() - meter->field_0x6c4[button]) * 0.5f;
    meter->field_0x6b8[button] =
        (meter->mpItemXY[button]->getInitSizeY() - meter->field_0x6d0[button]) * 0.5f;
    meter->mpItemXY[button]->resize(meter->field_0x6c4[button], meter->field_0x6d0[button]);
    meter->mpItemXY[button]->paneTrans(
        meter->mItemParams[button].pos_x + meter->field_0x6ac[button],
        meter->mItemParams[button].pos_y + meter->field_0x6b8[button]);
    meter->mpItemXYPane[button]->resize(
        meter->field_0x6c4[button], meter->field_0x6d0[button]);

    return HOOK_SKIP_ORIGINAL;
}

bool z_slot_item_ammo_count(u8 item, int& count) {
    if (item == dItemNo_BOOMERANG_e &&
        gale_bomb_combo_selected(kZItemSelectIndex))
    {
        const u8 bombBag = gale_bomb_combo_bag_slot(kZItemSelectIndex);
        if (bombBag == dItemNo_NONE_e) {
            return false;
        }
        count = dComIfGs_getBombNum(bombBag);
        return true;
    }

    switch (item) {
    case dItemNo_BOW_e:
    case dItemNo_LIGHT_ARROW_e:
    case dItemNo_ARROW_LV1_e:
    case dItemNo_ARROW_LV2_e:
    case dItemNo_ARROW_LV3_e:
    case dItemNo_HAWK_ARROW_e:
        count = dComIfGs_getArrowNum();
        return true;
    case dItemNo_BOMB_ARROW_e:
        count = dComIfGp_getSelectItemNum(kZItemSelectIndex);
        return true;
    case dItemNo_PACHINKO_e:
        count = dComIfGs_getPachinkoNum();
        return true;
    case dItemNo_NORMAL_BOMB_e:
        count = dComIfGs_getBombNum(0);
        return true;
    case dItemNo_WATER_BOMB_e:
        count = dComIfGs_getBombNum(1);
        return true;
    case dItemNo_POKE_BOMB_e:
        count = dComIfGs_getBombNum(2);
        return true;
    default:
        return false;
    }
}

bool z_slot_bow_lantern_combo(u8 item) {
    if (item != dItemNo_BOW_e) {
        return false;
    }
    const u8 selectedSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
    const u8 mixSlot = dComIfGs_getMixItemIndex(kZItemSelectIndex);
    if (selectedSlot == dItemNo_NONE_e || mixSlot == dItemNo_NONE_e) {
        return false;
    }
    const u8 selectedItem = dComIfGs_getItem(selectedSlot, false);
    const u8 mixItem = dComIfGs_getItem(mixSlot, false);
    return (selectedItem == dItemNo_BOW_e && mixItem == dItemNo_KANTERA_e) ||
           (selectedItem == dItemNo_KANTERA_e && mixItem == dItemNo_BOW_e);
}

void update_z_slot_ammo_display(dMeter2Draw_c* meter, u8 item) {
    g_secondaryZHudAmmoVisible = false;
    g_secondaryZHudLanternVisible =
        item == dItemNo_KANTERA_e || z_slot_bow_lantern_combo(item);
    if (meter == nullptr || meter->mpScreen == nullptr ||
        meter->mpItemR == nullptr)
    {
        g_secondaryZHudAmmoVisible = false;
        return;
    }

    if (g_secondaryZHudAmmoDigitsOwner != meter ||
        g_secondaryZHudAmmoScreen != meter->mpScreen)
    {
        // The meter's heap may have invalidated these pictures during a screen change.
        g_secondaryZHudAmmoDigits.fill(nullptr);
        g_secondaryZHudAmmoDigitsOwner = meter;
        g_secondaryZHudAmmoScreen = meter->mpScreen;
        g_secondaryZHudAmmoOwner = nullptr;
        g_secondaryZHudAmmoDigitCount = 0;
    }

    int count = 0;
    if (!z_slot_item_ammo_count(item, count)) {
        g_secondaryZHudAmmoVisible = false;
        return;
    }

    count = std::clamp(count, 0, 9999);
    const int digitCount = count >= 1000 ? 4 : count >= 100 ? 3 : 2;
    ResTIMG* zeroTexture = meter->getNumberTexture(0);
    if (zeroTexture == nullptr) {
        svc_log->error(mod_ctx, "failed to access the Z-slot ammo number texture");
        return;
    }

    J2DPane* itemPane = meter->mpItemR->getPanePtr();
    if (itemPane == nullptr) {
        return;
    }

    for (auto*& digit : g_secondaryZHudAmmoDigits) {
        if (digit != nullptr) {
            continue;
        }
        digit = JKR_NEW J2DPicture(zeroTexture);
        if (digit == nullptr) {
            svc_log->error(mod_ctx, "failed to allocate a Z-slot ammo digit");
            g_secondaryZHudAmmoVisible = false;
            return;
        }
        digit->setBlackWhite(
            JUtility::TColor(0, 0, 0, 0),
            JUtility::TColor(255, 255, 255, 255));
    }

    const Vec topLeft = itemPane->getGlbVtx(0);
    const Vec bottomRight = itemPane->getGlbVtx(3);
    const auto& numberParams =
        meter->mItemParams[dMeter2Draw_c::SELECT_Z_e];
    f32 numberOffsetX = numberParams.num_pos_x;
    f32 numberOffsetY = numberParams.num_pos_y;
    f32 numberScale = numberParams.num_scale;
    switch (item) {
    case dItemNo_PACHINKO_e:
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_BOMB_ARROW_e:
    case dItemNo_HAWK_ARROW_e:
    case dItemNo_BOOMERANG_e:
        numberOffsetX = 14.0f;
        numberOffsetY = -30.0f;
        numberScale = 0.75f;
        break;
    default:
        break;
    }
    const f32 digitSize = numberScale * 16.0f;
    const f32 left = numberOffsetX +
        ((topLeft.x + bottomRight.x) * 0.5f);
    const f32 top = numberOffsetY +
        ((topLeft.y + bottomRight.y) * 0.5f) + meter->mpItemR->getSizeY();
    int remaining = count;
    for (int i = digitCount - 1; i >= 0; --i) {
        J2DPicture* digit = g_secondaryZHudAmmoDigits[i];
        const int value = remaining % 10;
        remaining /= 10;
        ResTIMG* texture = meter->getNumberTexture(value);
        if (digit == nullptr || texture == nullptr ||
            digit->changeTexture(texture, 0) == nullptr)
        {
            svc_log->error(mod_ctx, "failed to update a Z-slot ammo digit");
            g_secondaryZHudAmmoVisible = false;
            return;
        }
        digit->show();
        digit->setAlpha(255);
    }
    g_secondaryZHudAmmoOwner = meter;
    g_secondaryZHudAmmoX = left;
    g_secondaryZHudAmmoY = top;
    g_secondaryZHudAmmoDigitSize = digitSize;
    g_secondaryZHudAmmoDigitCount = digitCount;
    g_secondaryZHudAmmoVisible = true;
}

DEFINE_HOOK_SYMBOL(
    "dMeter2Draw_c::~dMeter2Draw_c", void(dMeter2Draw_c*),
    ZSlotAmmoMeterDelete);

HookAction release_z_slot_ammo_digits(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr || meter != g_secondaryZHudAmmoDigitsOwner) {
        return HOOK_CONTINUE;
    }

    for (auto*& digit : g_secondaryZHudAmmoDigits) {
        if (digit != nullptr) {
            JKR_DELETE(digit);
            digit = nullptr;
        }
    }
    g_secondaryZHudAmmoDigitsOwner = nullptr;
    g_secondaryZHudAmmoScreen = nullptr;
    g_secondaryZHudAmmoOwner = nullptr;
    g_secondaryZHudAmmoVisible = false;
    g_secondaryZHudAmmoDigitCount = 0;
    return HOOK_CONTINUE;
}

DEFINE_HOOK(&dMeter2Draw_c::draw, ZSlotAmmoDraw);

void draw_z_slot_ammo_numbers(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr || meter->mpItemR == nullptr) {
        return;
    }

    J2DPane* itemPane = meter->mpItemR->getPanePtr();
    const f32 alpha = meter->mpItemR->getAlphaRate();
    if (itemPane == nullptr || !itemPane->isVisible() || alpha <= 0.0f) {
        return;
    }

    if (g_secondaryZHudLanternVisible) {
        if (g_secondaryZHudLanternMeter == nullptr) {
            g_secondaryZHudLanternMeter = JKR_NEW dKantera_icon_c();
            if (g_secondaryZHudLanternMeter == nullptr) {
                svc_log->error(mod_ctx, "failed to allocate the Z-slot Lantern meter");
                return;
            }
        }
        const Vec topLeft = itemPane->getGlbVtx(0);
        const Vec bottomRight = itemPane->getGlbVtx(3);
        g_secondaryZHudLanternMeter->setPos(
            ((topLeft.x + bottomRight.x) * 0.5f) + 9.0f,
            bottomRight.y);
        g_secondaryZHudLanternMeter->setScale(0.6f, 0.6f);
        g_secondaryZHudLanternMeter->setNowGauge(
            dComIfGs_getMaxOil(), dComIfGs_getOil());
        g_secondaryZHudLanternMeter->setAlphaRate(alpha);
        g_secondaryZHudLanternMeter->drawSelf();
        return;
    }

    if (!g_secondaryZHudAmmoVisible || meter != g_secondaryZHudAmmoOwner) {
        return;
    }
    const u8 digitAlpha = static_cast<u8>(
        std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    for (int i = 0; i < g_secondaryZHudAmmoDigitCount; ++i) {
        J2DPicture* digit = g_secondaryZHudAmmoDigits[i];
        if (digit == nullptr) {
            continue;
        }
        digit->setAlpha(digitAlpha);
        digit->draw(
            g_secondaryZHudAmmoX + g_secondaryZHudAmmoDigitSize * i,
            g_secondaryZHudAmmoY,
            g_secondaryZHudAmmoDigitSize,
            g_secondaryZHudAmmoDigitSize,
            false, false, false);
    }
}

void update_z_slot_hud_for_meter(dMeter2Draw_c* meter) {
    if (dComIfGp_isEnableNextStage()) {
        g_secondaryZHudOwner = nullptr;
        g_secondaryZHudPicture = nullptr;
        g_secondaryZHudSlot = dItemNo_NONE_e;
        g_secondaryZHudItem = dItemNo_NONE_e;
        g_secondaryZHudGaleBombCombo = false;
        return;
    }
    if (meter == nullptr || meter->mpScreen == nullptr) {
        return;
    }

    auto* zItemRoot = meter->mpScreen->search(MULTI_CHAR('item_r_n'));
    auto* midnaPane = meter->mpScreen->search(MULTI_CHAR('midona_n'));
    auto* dpadPane = meter->mpScreen->search(MULTI_CHAR('juji_n'));
    if (midnaPane != nullptr && g_zSlotMidnaScreen != meter->mpScreen) {
        g_zSlotMidnaScreen = meter->mpScreen;
        g_zSlotMidnaOriginalParent = midnaPane->getParentPane();
        g_zSlotMidnaOriginalScaleX = midnaPane->getScaleX();
        g_zSlotMidnaOriginalScaleY = midnaPane->getScaleY();
        g_zSlotMidnaOriginalTranslateX = midnaPane->getTranslateX();
        g_zSlotMidnaOriginalTranslateY = midnaPane->getTranslateY();
    }
    if (midnaPane != nullptr && zItemSlotEnabled() && dpadPane != nullptr) {
        if (midnaPane->getParentPane() != dpadPane) {
            dpadPane->appendChild(midnaPane);
            midnaPane->scale(g_drawHIO.mMidnaIconScale, g_drawHIO.mMidnaIconScale);
        }
        const f32 midnaWidth = midnaPane->getWidth() * midnaPane->getScaleX();
        const f32 midnaHeight = midnaPane->getHeight() * midnaPane->getScaleY();
        const f32 dpadWidth = dpadPane->getWidth() * dpadPane->getScaleX();
        const f32 dpadHeight = dpadPane->getHeight() * dpadPane->getScaleY();
        midnaPane->move(
            (dpadWidth - midnaWidth) * 0.5f - midnaWidth - 8.0f,
            (dpadHeight - midnaHeight) * 0.5f);
    } else if (midnaPane != nullptr && !zItemSlotEnabled() &&
               g_zSlotMidnaOriginalParent != nullptr &&
               midnaPane->getParentPane() != g_zSlotMidnaOriginalParent)
    {
        g_zSlotMidnaOriginalParent->appendChild(midnaPane);
        midnaPane->scale(g_zSlotMidnaOriginalScaleX, g_zSlotMidnaOriginalScaleY);
        midnaPane->translate(
            g_zSlotMidnaOriginalTranslateX, g_zSlotMidnaOriginalTranslateY);
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    const bool assigned = zItemSlotEnabled() &&
        secondaryItemAssigned(kZItemSelectIndex);
    const u8 assignedSlot = assigned
        ? g_secondarySelectedItems[kZItemSelectIndex] : dItemNo_NONE_e;
    if (meter->mpItemR == nullptr || meter->mpItemXYPane[2] == nullptr ||
        meter->mpLightXY[2] == nullptr)
    {
        return;
    }
    if (alink != nullptr && alink->checkWolf()) {
        update_z_slot_ammo_display(meter, dItemNo_NONE_e);
        if (assigned) {
            meter->mpItemR->hide();
            meter->mpLightXY[2]->hide();
            if (zItemRoot != nullptr) {
                zItemRoot->hide();
            }
        }
        g_secondaryZHudOwner = meter;
        g_secondaryZHudPicture = nullptr;
        g_secondaryZHudSlot = dItemNo_NONE_e;
        g_secondaryZHudItem = dItemNo_NONE_e;
        g_secondaryZHudGaleBombCombo = false;
        return;
    }

    auto* picture = static_cast<J2DPicture*>(meter->mpItemR->getPanePtr());
    if (picture == nullptr) {
        return;
    }
    if (g_secondaryZHudOwner != meter) {
        g_secondaryZHudOwner = meter;
        g_secondaryZHudPicture = nullptr;
        g_secondaryZHudSlot = dItemNo_NONE_e;
        g_secondaryZHudItem = dItemNo_NONE_e;
        g_secondaryZHudGaleBombCombo = false;
    }
    u8 displayItem = assigned
        ? secondaryDisplayItemForSlot(assignedSlot)
        : selected_item_for_button(kZItemSelectIndex);
    const bool galeBombComboActive =
        !assigned && gale_bomb_combo_selected(kZItemSelectIndex);
    if (displayItem == dItemNo_KANTERA_e &&
        z_slot_bow_lantern_combo(dItemNo_BOW_e))
    {
        displayItem = dItemNo_BOW_e;
    }
    if (displayItem != dItemNo_NONE_e) {
        meter->setItemParamZ(displayItem);
    }
    if (displayItem != dItemNo_NONE_e && zItemRoot != nullptr) {
        zItemRoot->show();
    }
    if (g_secondaryZHudSlot == assignedSlot &&
        g_secondaryZHudItem == displayItem &&
        g_secondaryZHudGaleBombCombo == galeBombComboActive &&
        g_secondaryZHudPicture == picture)
    {
        update_z_slot_ammo_display(meter, displayItem);
        return;
    }

    if (displayItem == dItemNo_NONE_e) {
        update_z_slot_ammo_display(meter, displayItem);
        meter->mpItemR->hide();
        meter->mpLightXY[2]->hide();
        if (zItemRoot != nullptr) {
            zItemRoot->hide();
        }
        g_secondaryZHudPicture = picture;
        g_secondaryZHudSlot = dItemNo_NONE_e;
        g_secondaryZHudItem = displayItem;
        g_secondaryZHudGaleBombCombo = false;
        return;
    }

    const SecondaryIconContext previousIconContext = g_secondaryIconContext;
    g_secondaryIconContext = assigned
        ? secondaryIconContextForSlot(assignedSlot) : SecondaryIconContext::None;
    const char* customIconResource =
        secondary_custom_icon_resource(g_secondaryIconContext);
    const u8 iconItem = galeBombComboActive
        ? dItemNo_NORMAL_BOMB_e : displayItem;
    int textureCount = 0;
    ResTIMG* texture =
        reinterpret_cast<ResTIMG*>(g_secondaryZHudTextureBuffers[0].data());
    if (customIconResource != nullptr) {
        ResourceBuffer icon = RESOURCE_BUFFER_INIT;
        const ModResult result =
            svc_resource->load(mod_ctx, customIconResource, &icon);
        if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
            svc_log->error(mod_ctx, "failed to load a custom Z HUD icon");
            svc_resource->free(mod_ctx, &icon);
            g_secondaryIconContext = previousIconContext;
            return;
        }
        if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
            svc_log->error(mod_ctx, "custom Z HUD icon has an invalid texture size");
            svc_resource->free(mod_ctx, &icon);
            g_secondaryIconContext = previousIconContext;
            return;
        }
        std::memcpy(texture, icon.data, icon.size);
        svc_resource->free(mod_ctx, &icon);
        DCStoreRangeNoSync(texture, kItemTextureBufferSize);
        if (picture->changeTexture(texture, 0) == nullptr) {
            svc_log->error(mod_ctx, "failed to apply a custom Z HUD icon");
            g_secondaryIconContext = previousIconContext;
            return;
        }
        meter->mpItemXYPane[2]->hide();
        textureCount = 1;
    } else {
        textureCount = dMeter2Info_readItemTexture(
            iconItem, g_secondaryZHudTextureBuffers[0].data(), picture,
            g_secondaryZHudTextureBuffers[1].data(), meter->mpItemXYPane[2],
            nullptr, nullptr, nullptr, nullptr, -1);
    }
    g_secondaryIconContext = previousIconContext;
    if (textureCount <= 0) {
        svc_log->error(mod_ctx, "failed to update the Z item HUD icon");
        return;
    }

    if (texture->width == 0 || texture->height == 0) {
        svc_log->error(mod_ctx, "Z item HUD icon has invalid dimensions");
        return;
    }
    if (galeBombComboActive) {
        auto* overlayTexture = reinterpret_cast<ResTIMG*>(
            g_secondaryZHudTextureBuffers[1].data());
        auto* overlayPicture = meter->mpItemXYPane[2];
        const int overlayTextureCount = dMeter2Info_readItemTexture(
            dItemNo_BOOMERANG_e, overlayTexture, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, -1);
        if (overlayTextureCount <= 0 || overlayTexture->width == 0 ||
            overlayTexture->height == 0 ||
            overlayPicture->changeTexture(overlayTexture, 0) == nullptr)
        {
            svc_log->error(mod_ctx, "failed to create the Gale and Bomb HUD icon");
            return;
        }
    }
    if (assigned) {
        picture->setBlackWhite(
            JUtility::TColor(0, 0, 0, 0),
            JUtility::TColor(255, 255, 255, 255));
    }
    if (galeBombComboActive) {
        meter->mpItemXYPane[2]->show();
    } else if (textureCount <= 1) {
        meter->mpItemXYPane[2]->hide();
    } else {
        meter->mpItemXYPane[2]->show();
    }

    const f32 scale = g_drawHIO.mItemScaleAdjustON
        ? g_drawHIO.mItemScalePercent / 100.0f
        : dItem_data::getTexScale(iconItem) / 100.0f;
    const f32 bombIconScale = galeBombComboActive ? 0.78f : 1.0f;
    meter->field_0x6c4[2] =
        scale * bombIconScale *
        ((texture->width * meter->mpItemR->getInitSizeX()) / 48.0f);
    meter->field_0x6d0[2] =
        scale * bombIconScale *
        ((texture->height * meter->mpItemR->getInitSizeY()) / 48.0f);
    meter->field_0x6ac[2] =
        (meter->mpItemR->getInitSizeX() - meter->field_0x6c4[2]) * 0.5f;
    meter->field_0x6b8[2] =
        (meter->mpItemR->getInitSizeY() - meter->field_0x6d0[2]) * 0.5f;
    meter->mpItemR->resize(meter->field_0x6c4[2], meter->field_0x6d0[2]);
    meter->mpItemR->paneTrans(
        g_drawHIO.mButtonZItemPosX + meter->field_0x6ac[2] +
            (galeBombComboActive ? meter->field_0x6c4[2] * 0.12f : 0.0f),
        g_drawHIO.mButtonZItemPosY + meter->field_0x6b8[2] +
            (galeBombComboActive ? meter->field_0x6d0[2] * 0.12f : 0.0f));
    if (galeBombComboActive) {
        auto* overlayTexture = reinterpret_cast<ResTIMG*>(
            g_secondaryZHudTextureBuffers[1].data());
        const f32 overlayScale = g_drawHIO.mItemScaleAdjustON
            ? g_drawHIO.mItemScalePercent / 100.0f
            : dItem_data::getTexScale(dItemNo_BOOMERANG_e) / 100.0f;
        const f32 overlayWidth = overlayScale *
            (overlayTexture->width * meter->mpItemR->getInitSizeX() / 48.0f);
        const f32 overlayHeight = overlayScale *
            (overlayTexture->height * meter->mpItemR->getInitSizeY() / 48.0f);
        meter->mpItemXYPane[2]->resize(overlayWidth, overlayHeight);
        meter->mpItemXYPane[2]->translate(
            g_drawHIO.mButtonZItemPosX +
                (meter->mpItemR->getInitSizeX() - overlayWidth) * 0.5f,
            g_drawHIO.mButtonZItemPosY +
                (meter->mpItemR->getInitSizeY() - overlayHeight) * 0.5f);
    } else {
        meter->mpItemXYPane[2]->resize(
            meter->field_0x6c4[2], meter->field_0x6d0[2]);
    }
    update_z_slot_ammo_display(meter, displayItem);
    meter->mpItemR->show();
    meter->mpLightXY[2]->show();
    g_secondaryZHudPicture = picture;
    g_secondaryZHudSlot = assignedSlot;
    g_secondaryZHudItem = displayItem;
    g_secondaryZHudGaleBombCombo = galeBombComboActive;
}

void update_z_slot_hud(ModContext*, void* args, void*, void*) {
    update_z_slot_hud_for_meter(mods::arg<dMeter2Draw_c*>(args, 0));
}

void refresh_secondary_hud_assignments(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2_c*>(args, 0);
    if (meter == nullptr || meter->mpMeterDraw == nullptr) {
        return;
    }
    update_z_slot_hud_for_meter(meter->mpMeterDraw);

    if (g_secondaryHudOwner != meter) {
        g_secondaryHudOwner = meter;
        g_secondaryHudAssignmentsInitialized = false;
    }

    if (!g_secondaryHudAssignmentsInitialized) {
        g_secondaryHudAssignments = g_secondarySelectedItems;
        for (int button = 0; button < MAX_SELECT_ITEM; ++button) {
            g_secondaryHudRefreshPending[button] = secondaryItemAssigned(button);
        }
        g_secondaryHudAssignmentsInitialized = true;
        return;
    }

    for (int button = 0; button < 2; ++button) {
        const u8 slot = g_secondarySelectedItems[button];
        if (g_secondaryHudAssignments[button] != slot) {
            g_secondaryHudAssignments[button] = slot;
            g_secondaryHudRefreshPending[button] = true;
        }
        if (!g_secondaryHudRefreshPending[button] || meter->field_0x128 != 0 ||
            meter->mpMeterDraw->mpItemXY[button] == nullptr ||
            meter->mpMeterDraw->mpItemXYPane[button] == nullptr)
        {
            continue;
        }

        const u8 item = secondaryItemAssigned(button)
                            ? secondaryDisplayItemForSlot(slot)
                            : meter->mItemStatus[button * 2];
        const u8 status = meter->mItemStatus[button * 2 + 1];
        meter->mpMeterDraw->drawButtonXY(
            button, item, status, true, false);
        g_secondaryHudRefreshPending[button] = false;
    }
    g_secondaryHudAssignments[kZItemSelectIndex] =
        g_secondarySelectedItems[kZItemSelectIndex];
    g_secondaryHudRefreshPending[kZItemSelectIndex] = false;
}

HookAction assign_secondary_z_from_ring(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (ring == nullptr || isSecondaryRing(ring)) {
        return HOOK_CONTINUE;
    }

    if (ring->mStatus != dMenu_Ring_c::STATUS_WAIT ||
        ring->mOldStatus == dMenu_Ring_c::STATUS_EXPLAIN_FORCE ||
        ring->mOldStatus == dMenu_Ring_c::STATUS_EXPLAIN ||
        ring->mpItemExplain == nullptr || ring->mpItemExplain->getStatus() != 0 ||
        ring->mPlayerIsWolf || ring->mCurrentSlot >= MAX_ITEM_SLOTS)
    {
        return HOOK_CONTINUE;
    }

    const u8 selectedSlot = ring->mItemSlots[ring->mCurrentSlot];
    if (selectedSlot == dItemNo_NONE_e || selectedSlot >= MAX_ITEM_SLOTS) {
        return HOOK_CONTINUE;
    }
    const u8 selectedItem = dComIfGs_getItem(selectedSlot, false);
    if (selectedItem == dItemNo_NONE_e) {
        return HOOK_CONTINUE;
    }

    const bool zTriggered = zItemSlotEnabled() &&
                            mDoCPd_c::getTrigZ(PAD_1) != 0;
    const bool rTriggered = zItemSlotEnabled() &&
                            mDoCPd_c::getTrigR(PAD_1) != 0;
    if (rTriggered && !zTriggered &&
        toggle_z_gale_bomb_combo(ring, selectedSlot, selectedItem))
    {
        return HOOK_SKIP_ORIGINAL;
    }

    if (rTriggered && !zTriggered &&
        toggle_z_bow_combo(ring, selectedSlot, selectedItem))
    {
        return HOOK_SKIP_ORIGINAL;
    }

    if (!zTriggered) {
        return HOOK_CONTINUE;
    }

    u8 mixItemSlot = dItemNo_NONE_e;
    for (int button = 0; button < 2; ++button) {
        if (dComIfGs_getSelectItemIndex(button) == selectedSlot) {
            mixItemSlot = dComIfGs_getMixItemIndex(button);
            break;
        }
    }

    dComIfGs_setSelectItemIndex(kZItemSelectIndex, selectedSlot);
    dComIfGs_setMixItemIndex(kZItemSelectIndex, mixItemSlot);
    for (int button = 0; button < kZItemSelectIndex; ++button) {
        const u8 assignedSlot = dComIfGs_getSelectItemIndex(button);
        const u8 assignedMixSlot = dComIfGs_getMixItemIndex(button);
        if (assignedSlot == selectedSlot) {
            dComIfGs_setSelectItemIndex(button, dItemNo_NONE_e);
            dComIfGs_setMixItemIndex(button, dItemNo_NONE_e);
            ring->setSelectItem(button, dItemNo_NONE_e);
        } else if (assignedMixSlot == selectedSlot) {
            dComIfGs_setMixItemIndex(button, dItemNo_NONE_e);
            ring->setSelectItem(button, selected_item_for_button(button));
        }
    }
    const u8 displayItem = selected_item_for_button(kZItemSelectIndex);
    clear_secondary_assignment(kZItemSelectIndex);
    sync_primary_ring_item_selection(ring);
    begin_z_slot_assignment_animation(
        ring, ring->mCurrentSlot, selectedSlot, displayItem);
    if (ring->mpItemExplain->getStatus() == 0) {
        ring->mStatus = dMenu_Ring_c::STATUS_WAIT;
        ring->stick_wait_init();
    }
    return HOOK_SKIP_ORIGINAL;
}

void begin_z_slot_assignment_animation(
    dMenu_Ring_c* ring, u8 sourceSlot, u8 savedSelection, u8 displayItem)
{
    ring->field_0x6ac = sourceSlot;
    ring->field_0x6b3 = kZItemSelectIndex;
    ring->field_0x6b4[kZItemSelectIndex] = displayItem;
    ring->field_0x6b8[kZItemSelectIndex] = dItemNo_NONE_e;
    ring->field_0x674[kZItemSelectIndex] = 1;
    ring->field_0x538[kZItemSelectIndex] = g_ringHIO.mSelectItemScale;

    if (isSecondaryRing(ring)) {
        ring->field_0x518[kZItemSelectIndex] =
            g_secondaryWheelSlotPosX[sourceSlot];
        ring->field_0x528[kZItemSelectIndex] =
            g_secondaryWheelSlotPosY[sourceSlot];
        ring->setSelectItem(kZItemSelectIndex, displayItem);
        dMeter2Info_set2DVibrationM();
        Z2GetAudioMgr()->seStart(
            Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    } else {
        ring->setJumpItem(true);
    }
    if (!isSecondaryRing(ring)) {
        ring->field_0x6b4[kZItemSelectIndex] = savedSelection;
        ring->setSelectItem(kZItemSelectIndex, displayItem);
    }
}

HookAction preserve_secondary_z_assignment(
    ModContext*, void* args, void*, void*)
{
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    const int button = mods::arg<int>(args, 1);
    if (!isSecondaryRing(ring) || button != kZItemSelectIndex) {
        return HOOK_CONTINUE;
    }

    ring->field_0x674[kZItemSelectIndex] = 0;
    return HOOK_SKIP_ORIGINAL;
}

HookAction activate_vanilla_z_slot_item(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    auto* result = static_cast<int*>(retval);
    if (!zItemSlotEnabled() || alink == nullptr || alink->checkWolf() ||
        secondaryItemAssigned(kZItemSelectIndex) || result == nullptr ||
        mDoCPd_c::getTrigZ(PAD_1) == 0)
    {
        return HOOK_CONTINUE;
    }

    const int itemNo = mods::arg<int>(args, 1);
    const u8 zItem = selected_item_for_button(kZItemSelectIndex);
    if (zItem == dItemNo_NONE_e || !alink->checkGroupItem(itemNo, zItem))
    {
        return HOOK_CONTINUE;
    }

    alink->mSelectItemId = kZItemSelectIndex;
    *result = 1;
    return HOOK_SKIP_ORIGINAL;
}

void handle_vanilla_z_item_change(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    auto* result = static_cast<BOOL*>(retval);
    if (!zItemSlotEnabled() || alink == nullptr || alink->checkWolf() ||
        secondaryItemAssigned(kZItemSelectIndex) || result == nullptr ||
        *result != 0 || g_secondaryRing != nullptr || g_primaryRingOpen ||
        mDoCPd_c::getTrigZ(PAD_1) == 0 ||
        mDoCPd_c::getTrigX(PAD_1) != 0 ||
        mDoCPd_c::getTrigY(PAD_1) != 0)
    {
        return;
    }

    if (!alink->checkModeFlg(4) || alink->checkEquipAnime() ||
        alink->checkBoomerangThrowAnime() || alink->checkCopyRodThrowAnime() ||
        alink->checkKandelaarSwingAnime() || alink->checkKandelaarSwingAnime() ||
        alink->swordTrigger())
    {
        return;
    }

    const u8 selectedSlot = dComIfGs_getSelectItemIndex(kZItemSelectIndex);
    if (selectedSlot == dItemNo_NONE_e) {
        return;
    }

    const u8 selectedItem = dComIfGs_getItem(selectedSlot, false);
    if (selectedItem == dItemNo_NONE_e) {
        return;
    }

    const int procType = alink->checkNewItemChange(kZItemSelectIndex);
    if (procType != 0) {
        *result = alink->changeItemTriggerKeepProc(kZItemSelectIndex, procType);
    }
    alink->mSelectItemId = kZItemSelectIndex;
}

void clear_secondary_assignment(int button) {
    if (!secondaryItemAssigned(button)) {
        return;
    }

    g_secondarySelectedItems[button] = dItemNo_NONE_e;
    if (!secondaryTunicAssigned(0) && !secondaryTunicAssigned(1) &&
        !secondaryTunicAssigned(kZItemSelectIndex))
    {
        set_goron_tunic_active(false);
    }
    if (!secondaryHoverBootsAssigned(0) && !secondaryHoverBootsAssigned(1) &&
        !secondaryHoverBootsAssigned(kZItemSelectIndex))
    {
        disable_hover_boots();
    }
}

void clear_duplicate_z_assignment_for_item(
    dMenu_Ring_c* ring, int button, u8 slot, u8 item)
{
    if (ring == nullptr || button < 0 || button >= kZItemSelectIndex ||
        item == dItemNo_NONE_e || !zItemSlotEnabled())
    {
        return;
    }

    auto* player = daAlink_getAlinkActorClass();
    const auto sameItemGroup = [player, item](u8 assignedItem) {
        return assignedItem != dItemNo_NONE_e &&
            (assignedItem == item ||
             (player != nullptr && player->checkGroupItem(item, assignedItem)));
    };

    const bool duplicateSecondary =
        secondaryItemAssigned(kZItemSelectIndex) &&
        (g_secondarySelectedItems[kZItemSelectIndex] == slot ||
         sameItemGroup(secondaryItemIdForSlot(
             g_secondarySelectedItems[kZItemSelectIndex])));
    const bool duplicateVanilla =
        sameItemGroup(selected_item_for_button(kZItemSelectIndex));
    if (!duplicateSecondary && !duplicateVanilla) {
        return;
    }

    if (duplicateSecondary) {
        clear_secondary_assignment(kZItemSelectIndex);
    }
    if (duplicateVanilla) {
        dComIfGs_setSelectItemIndex(kZItemSelectIndex, dItemNo_NONE_e);
        dComIfGs_setMixItemIndex(kZItemSelectIndex, dItemNo_NONE_e);
    }

    if (isSecondaryRing(ring)) {
        const u8 zItem = selected_item_for_button(kZItemSelectIndex);
        ring->field_0x6ac = dItemNo_NONE_e;
        ring->field_0x6b8[kZItemSelectIndex] = dItemNo_NONE_e;
        ring->field_0x6b4[kZItemSelectIndex] = zItem;
        ring->field_0x674[kZItemSelectIndex] = 0;
        ring->setSelectItem(kZItemSelectIndex, zItem);
    } else {
        sync_primary_ring_item_selection(ring);
        ring->setSelectItem(
            kZItemSelectIndex, selected_item_for_button(kZItemSelectIndex));
    }
}

void clear_secondary_assignment_for_vanilla_ring(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (ring == nullptr || isSecondaryRing(ring)) {
        return;
    }

    const int button = ring->field_0x6b3;
    if (button < 0 || button >= kSecondaryAssignmentCount ||
        (button == kZItemSelectIndex && !zItemSlotEnabled()))
    {
        return;
    }
    if (secondaryItemAssigned(button)) {
        clear_secondary_assignment(button);
    }
    if (button < kZItemSelectIndex) {
        const u8 pendingItem = ring->field_0x6b4[button];
        const u8 pendingMixItem = ring->field_0x6b8[button];
        const u8 pendingCursor =
            button == 0 ? ring->mXButtonSlot : ring->mYButtonSlot;
        // X/Y indices are committed after the item-set animation, so inspect
        // the pending ring selection instead of the still-saved assignment.
        clear_duplicate_z_assignment_for_item(
            ring, button, dItemNo_NONE_e,
            ring->getItem(pendingItem, pendingMixItem));
        ring->field_0x6b4[button] = pendingItem;
        ring->field_0x6b8[button] = pendingMixItem;
        if (button == 0) {
            ring->mXButtonSlot = pendingCursor;
        } else {
            ring->mYButtonSlot = pendingCursor;
        }
        ring->setSelectItem(button, ring->getItem(pendingItem, pendingMixItem));
    }
}

void finish_hover_boots_hud_icon(ModContext*, void*, void*, void*) {
    g_secondaryIconContext = SecondaryIconContext::None;
}
