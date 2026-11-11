// Dawnlight Glider geometry/presentation uses CC0; its texture is from BeZide's custom mod
// and included with permission.
namespace manual_glider_visual {
namespace {

constexpr std::size_t kMaxBmdBytes = 8 * 1024 * 1024;
constexpr f32 kGliderRaisedOffset = 12.0f;
JKRExpHeap* s_heap = nullptr;
std::unique_ptr<u8[]> s_heapStorage;
J3DModel* s_model = nullptr;
J3DDrawBuffer* s_opaque = nullptr;
J3DDrawBuffer* s_translucent = nullptr;
bool s_attempted = false;
Mtx s_carrierTransform{};
bool s_carrierTransformValid = false;
bool s_carrierTransformErrorLogged = false;
using ForgetModelMatrices = void (*)(J3DModel*);
ForgetModelMatrices s_forgetMatrices = nullptr;

struct CurrentHeap {
    JKRHeap* previous;
    explicit CurrentHeap(JKRHeap* heap) : previous(heap->becomeCurrentHeap()) {}
    ~CurrentHeap() {
        if (previous != nullptr) previous->becomeCurrentHeap();
    }
};

bool valid_bmd(const void* bytes, std::size_t size) {
    if (bytes == nullptr || size < 32 || size > kMaxBmdBytes) return false;
    const auto* p = static_cast<const u8*>(bytes);
    auto read_be32 = [](const u8* value) {
        return (u32(value[0]) << 24) | (u32(value[1]) << 16) |
               (u32(value[2]) << 8) | value[3];
    };
    if (std::memcmp(p, "J3D2bmd3", 8) != 0 ||
        read_be32(p + 8) != size || read_be32(p + 12) != 8)
    {
        return false;
    }

    constexpr const char* tags[] = {
        "INF1", "VTX1", "EVP1", "DRW1", "JNT1", "SHP1", "MAT3", "TEX1"};
    constexpr u32 minimum[] = {24, 64, 28, 20, 24, 44, 132, 20};
    u32 seen = 0;
    std::size_t offset = 32;
    for (u32 block = 0; block < 8; ++block) {
        if (size - offset < 8) return false;
        const u32 length = read_be32(p + offset + 4);
        if (length < 8 || length > size - offset || length % 4 != 0) return false;
        u32 index = 0;
        for (; index < 8 && std::memcmp(p + offset, tags[index], 4) != 0; ++index) {}
        if (index == 8 || (seen & (1u << index)) != 0 || length < minimum[index])
            return false;
        seen |= 1u << index;
        if (index >= 4 && index <= 6 && p[offset + 8] == 0 && p[offset + 9] == 0)
            return false;
        offset += length;
    }
    return seen == 255 && offset == size;
}

void release_model() {
    if (s_model != nullptr && s_forgetMatrices != nullptr) s_forgetMatrices(s_model);
    s_model = nullptr;
    s_opaque = nullptr;
    s_translucent = nullptr;
    if (s_heap != nullptr) s_heap->destroy();
    s_heap = nullptr;
    s_heapStorage.reset();
}

bool create_model_heap(u32 size) {
    if (size < 32 || size > kMaxBmdBytes) return false;
    const u32 heapSize = 8 * 1024 * 1024 + size * 4;
    s_heapStorage.reset(new (std::nothrow) u8[heapSize + 31]);
    if (!s_heapStorage) return false;
    auto* memory = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(s_heapStorage.get()) + 31) & ~uintptr_t{31});
    s_heap = JKRExpHeap::create(memory, heapSize, JKRHeap::getRootHeap(), false);
    if (s_heap == nullptr) s_heapStorage.reset();
    return s_heap != nullptr;
}

bool load_model(void* bytes, u32 size) {
    if (!valid_bmd(bytes, size)) return false;
    auto* data = J3DModelLoaderDataBase::load(bytes, 0x59020010);
    if (data == nullptr || data->getJointNum() == 0 || data->getShapeNum() == 0 ||
        data->getMaterialNum() == 0)
    {
        return false;
    }
    auto* model = JKR_NEW J3DModel();
    if (model == nullptr || model->entryModelData(data, 0, 1) != kJ3DError_Success)
        return false;
    s_opaque = JKR_NEW J3DDrawBuffer();
    s_translucent = JKR_NEW J3DDrawBuffer();
    if (s_opaque == nullptr || s_translucent == nullptr ||
        s_opaque->allocBuffer(32) != kJ3DError_Success ||
        s_translucent->allocBuffer(32) != kJ3DError_Success)
    {
        return false;
    }
    s_translucent->setZSort();
    s_model = model;
    return true;
}

bool load_bundled_model() {
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    if (svc_resource->load(mod_ctx, "DawnlightGlider.bmd", &buffer) != MOD_OK)
        return false;
    bool loaded = false;
    if (valid_bmd(buffer.data, buffer.size) &&
        create_model_heap(static_cast<u32>(buffer.size)))
    {
        CurrentHeap scope(s_heap);
        auto* bytes = s_heap->alloc(static_cast<u32>(buffer.size), 32);
        if (bytes != nullptr) {
            std::memcpy(bytes, buffer.data, buffer.size);
            loaded = load_model(bytes, static_cast<u32>(buffer.size));
        }
    }
    svc_resource->free(mod_ctx, &buffer);
    return loaded;
}

bool prepare_model() {
    if (!s_attempted) {
        s_attempted = true;
        void* address = nullptr;
        if (svc_hook && svc_hook->resolve &&
            svc_hook->resolve(mod_ctx, "J3DModel::forgetMtx", &address, nullptr) == MOD_OK)
        {
            s_forgetMatrices = reinterpret_cast<ForgetModelMatrices>(address);
        }
        if (!load_bundled_model()) {
            release_model();
            svc_log->error(mod_ctx, "failed to load the bundled Dawnlight Glider model");
        }
    }
    return s_model != nullptr;
}

bool draw_model(Mtx transform, dKy_tevstr_c* lighting) {
    if (!s_model) return false;
    static Mtx presentedView{};
    CurrentHeap scope(s_heap);
    const J3DSys saved = j3dSys;
    s_opaque->frameInit();
    s_translucent->frameInit();
    j3dSys.setDrawBuffer(s_opaque, 0);
    j3dSys.setDrawBuffer(s_translucent, 1);
    MTXConcat(saved.mViewMtx, transform, j3dSys.getViewMtx());
    s_translucent->setZMtx(s_model->getBaseTRMtx());
    j3dSys.reinitGX();
    if (lighting != nullptr) g_env_light.setLightTevColorType_MAJI(s_model, lighting);
    s_model->unlock();
    s_model->update();
    s_model->lock();
    s_model->viewCalc();
    MTXCopy(j3dSys.getViewMtx(), presentedView);
    for (u16 i = 0; i < s_model->getModelData()->getShapeNum(); ++i)
        s_model->getShapePacket(i)->setBaseMtxPtr(&presentedView);
    j3dSys.setDrawModeOpaTexEdge();
    s_opaque->draw();
    j3dSys.setDrawModeXlu();
    s_translucent->draw();
    j3dSys = saved;
    j3dSys.reinitGX();
    return true;
}

struct GliderPacket : J3DPacket {
    fpc_ProcID owner = fpcM_ERROR_PROCESS_ID_e;
    bool active = false;

    void draw() override {
        if (!active || !prepare_model()) return;
        auto* actor = fopAcM_SearchByID(owner);
        if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_ALINK_e) return;
        auto* link = static_cast<daAlink_c*>(actor);
        if (!link->mpLinkModel || link->checkPlayerNoDraw() ||
            link->checkStatusWindowDraw())
        {
            return;
        }
        Mtx transform{};
        if (!s_carrierTransformValid ||
            !svc_interp->lookup_replacement_mtx(&s_carrierTransform, transform))
        {
            MTXCopy(s_carrierTransform, transform);
        }
        transform[1][3] += kGliderRaisedOffset;
        draw_model(transform, &link->tevStr);
    }
};

GliderPacket s_packet;

bool glider_active(daAlink_c* link) {
    return link != nullptr && g_manualCuccoGlideActive &&
        g_manualGlideActorId != fpcM_ERROR_PROCESS_ID_e && glideItem() == 1 &&
        !link->checkWolf() && !link->mLinkAcch.ChkGroundHit() &&
        link->mGrabItemAcKeep.getActor() == fopAcM_SearchByID(g_manualGlideActorId) &&
        link->mProcID == daAlink_c::PROC_AUTO_JUMP && !link->checkEventRun();
}

void queue_packet() {
    auto* list = dComIfGd_getOpaList();
    if (list == nullptr || list->mpBuffer == nullptr || list->getEntryTableSize() == 0)
        return;
    for (auto* queued = list->mpBuffer[0]; queued; queued = queued->getNextPacket()) {
        if (queued == &s_packet) return;
    }
    list->entryImm(&s_packet, 0);
}
}

void update(daAlink_c* link) {
    if (!glider_active(link) || !prepare_model()) return;
    auto* actor = static_cast<ni_class*>(fopAcM_SearchByID(g_manualGlideActorId));
    if (actor == nullptr) return;
    if (auto* emitter = dComIfGp_particle_getEmitter(actor->mHaneEmitterID)) {
        emitter->setRate(0.0f);
        emitter->deleteAllParticle();
    }
    actor->mSound.stopAllSounds(0);
}

void after_player_draw(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_packet.active = false;
    if (!glider_active(link) || !prepare_model() ||
        link->checkPlayerNoDraw() || link->checkStatusWindowDraw())
    {
        return;
    }
    auto* carrier = static_cast<ni_class*>(fopAcM_SearchByID(g_manualGlideActorId));
    if (carrier == nullptr || carrier->mpMorf == nullptr ||
        carrier->mpMorf->getModel() == nullptr)
    {
        return;
    }
    const MtxP carrierTransform =
        carrier->mpMorf->getModel()->getBaseTRMtx();
    MTXCopy(carrierTransform, s_carrierTransform);
    if (svc_interp->record_mtx_keyed(
            mod_ctx, s_carrierTransform, &s_carrierTransform) != MOD_OK &&
        !s_carrierTransformErrorLogged)
    {
        s_carrierTransformErrorLogged = true;
        svc_log->error(mod_ctx, "failed to record manual Glider interpolation matrix");
    }
    s_carrierTransformValid = true;
    s_packet.owner = fopAcM_GetID(link);
    s_packet.active = true;
    queue_packet();
}

void after_carrier_execute(ModContext*, void* args, void*, void*) {
    auto* actor = static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    if (actor == nullptr || g_manualGlideActorId == fpcM_ERROR_PROCESS_ID_e ||
        fopAcM_GetID(actor) != g_manualGlideActorId ||
        fopAcM_GetName(actor) != fpcNm_NI_e || glideItem() != 1 ||
        !g_manualCuccoGlideActive)
    {
        return;
    }
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || link->mGrabItemAcKeep.getActor() != actor) return;

    auto* carrier = static_cast<ni_class*>(actor);
    if (auto* emitter = dComIfGp_particle_getEmitter(carrier->mHaneEmitterID)) {
        emitter->setRate(0.0f);
        emitter->deleteAllParticle();
    }
    carrier->mSound.stopAllSounds(0);
}

void after_draw_hand(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!glider_active(link) || !link->mpLinkHandModel) return;
    auto* data = link->mpLinkHandModel->getModelData();
    if (!data || data->getMaterialNum() <= 6) return;
    auto* leftMaterial = data->getMaterialNodePointer(0);
    auto* rightMaterial = data->getMaterialNodePointer(6);
    if (!leftMaterial || !rightMaterial) return;
    auto* left = leftMaterial->getShape();
    auto* right = rightMaterial->getShape();
    if (!left || !right) return;
    if (link->field_0x06d0) link->field_0x06d0->hide();
    if (link->field_0x06d4) link->field_0x06d4->hide();
    link->field_0x06d0 = left;
    link->field_0x06d4 = right;
    left->show();
    right->show();
}

HookAction before_carrier_draw(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (process != nullptr && g_manualGlideActorId != fpcM_ERROR_PROCESS_ID_e &&
        fpcM_GetID(process) == g_manualGlideActorId && glideItem() == 1 &&
        prepare_model())
    {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void shutdown() {
    s_packet.active = false;
    s_carrierTransformValid = false;
    if (svc_interp->forget_mtx(mod_ctx, &s_carrierTransform) != MOD_OK &&
        !s_carrierTransformErrorLogged)
    {
        s_carrierTransformErrorLogged = true;
        svc_log->error(mod_ctx, "failed to release manual Glider interpolation matrix");
    }
    release_model();
    s_attempted = false;
}

DEFINE_HOOK(&daAlink_c::draw, ManualGliderPlayerDraw);
DEFINE_HOOK(&daAlink_c::setDrawHand, ManualGliderDrawHand);
DEFINE_HOOK(&fpcLf_Draw, ManualGliderCarrierDraw);
}
