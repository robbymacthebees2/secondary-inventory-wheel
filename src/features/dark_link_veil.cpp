#include "dark_link_veil.hpp"
#include "dark_link_veil_mesh.hpp"
#include "mods/service.hpp"
#include "mods/svc/gfx.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/interp.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "m_Do/m_Do_mtx.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_view.h"

#include <chrono>
#include <atomic>
#include <cstring>
#include <mutex>

namespace dark_link_veil {
namespace {
DEFINE_HOOK(&dComIfGd_drawXluListDark, DarkVeilTranslucentHook);
DEFINE_HOOK(&dComIfGd_drawListItem3d, DarkVeilCollectionHook);
using namespace dawnlight::dark_veil;
using Clock=std::chrono::steady_clock;
using LookupPresentationMatrix=bool (*)(const void*,Mtx);
LookupPresentationMatrix sPresented=nullptr;
GfxDrawTypeHandle sDraw=0;
GfxStageHookHandle sStage=0,sBegin=0,sEnd=0;
bool sHook=false,sCollectionHook=false,sCollectionDrawing=false;
fpc_ProcID sOwner=fpcM_ERROR_PROCESS_ID_e;
std::atomic_bool sPipelineWarmed=false;
std::atomic_bool sPipelineWarmupQueued=false;
struct Animation {
    Motion motion;
    Clock::time_point last{},origin{};
    fpc_ProcID owner=fpcM_ERROR_PROCESS_ID_e;
    float fade=0;
    void reset() { *this={}; }
};
Animation sWorld,sCollection;
Mtx sCollectionView{};
Mtx44 sCollectionProjection{};
Vec3 sCollectionEye{};
struct GpuVertex { float clip[4],field[3],opacity,color[3]; };
struct Draw { GfxRange vertices; uint32_t count; };
Draw sPending{};
bool sHasPending=false;
std::vector<Vertex> sMesh;
std::vector<GpuVertex> sVertices;

// GPU objects are used on the worker only, with shutdown serialized against it.
std::mutex sGpuMutex;
WGPUShaderModule sShader=nullptr;
struct Pipeline { uint64_t layout; bool reversed; WGPURenderPipeline handle; };
std::vector<Pipeline> sPipelines;
constexpr const char* kShader=
#include "dark_link_veil_shader.inc"
;
WGPUStringView label(const char* s) { return {s,std::strlen(s)}; }
WGPURenderPipeline pipeline_for(const GfxDrawContext& ctx) {
    for (const auto& p:sPipelines)
        if (p.layout==ctx.layout.key && p.reversed==ctx.uses_reversed_z) return p.handle;
    if (!sShader) {
        WGPUShaderSourceWGSL source=WGPU_SHADER_SOURCE_WGSL_INIT;
        source.code=label(kShader);
        WGPUShaderModuleDescriptor desc=WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        desc.nextInChain=&source.chain;
        sShader=wgpuDeviceCreateShaderModule(ctx.device,&desc);
    }
    if (!sShader) return nullptr;
    WGPUVertexAttribute attributes[4]={WGPU_VERTEX_ATTRIBUTE_INIT,WGPU_VERTEX_ATTRIBUTE_INIT,WGPU_VERTEX_ATTRIBUTE_INIT,WGPU_VERTEX_ATTRIBUTE_INIT};
    attributes[0].format=WGPUVertexFormat_Float32x4;
    attributes[1].format=WGPUVertexFormat_Float32x3; attributes[1].offset=offsetof(GpuVertex,field);
    attributes[2].format=WGPUVertexFormat_Float32; attributes[2].offset=offsetof(GpuVertex,opacity);
    attributes[3].format=WGPUVertexFormat_Float32x3; attributes[3].offset=offsetof(GpuVertex,color);
    for (unsigned i=0;i<4;++i) attributes[i].shaderLocation=i;
    WGPUVertexBufferLayout buffer=WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    buffer.arrayStride=sizeof(GpuVertex); buffer.attributeCount=4; buffer.attributes=attributes;
    WGPUBlendState blend=WGPU_BLEND_STATE_INIT;
    blend.color.srcFactor=WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor=WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.srcFactor=WGPUBlendFactor_Zero; blend.alpha.dstFactor=WGPUBlendFactor_One;
    WGPUColorTargetState targets[GFX_MAX_COLOR_ATTACHMENTS];
    WGPUFragmentState fragment=WGPU_FRAGMENT_STATE_INIT;
    fragment.module=sShader; fragment.entryPoint=label("fs");
    fragment.targetCount=gfx_init_color_target_states(&ctx.layout,targets,&blend,
        WGPUColorWriteMask_Red|WGPUColorWriteMask_Green|WGPUColorWriteMask_Blue);
    fragment.targets=targets;
    WGPUDepthStencilState depth=WGPU_DEPTH_STENCIL_STATE_INIT;
    depth.format=ctx.layout.depth_stencil_format;
    depth.depthWriteEnabled=WGPUOptionalBool_False;
    depth.depthCompare=ctx.uses_reversed_z ? WGPUCompareFunction_GreaterEqual:WGPUCompareFunction_LessEqual;
    WGPURenderPipelineDescriptor desc=WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.label=label("Dark Link attached veil");
    desc.vertex.module=sShader; desc.vertex.entryPoint=label("vs");
    desc.vertex.bufferCount=1; desc.vertex.buffers=&buffer;
    desc.primitive.topology=WGPUPrimitiveTopology_TriangleList;
    desc.primitive.cullMode=WGPUCullMode_None;
    desc.depthStencil=depth.format==WGPUTextureFormat_Undefined ? nullptr:&depth;
    desc.multisample.count=ctx.layout.sample_count;
    desc.fragment=&fragment;
    auto pipeline=wgpuDeviceCreateRenderPipeline(ctx.device,&desc);
    if (pipeline) sPipelines.push_back({ctx.layout.key,ctx.uses_reversed_z,pipeline});
    return pipeline;
}
void draw(ModContext*,const GfxDrawContext* ctx,const void* data,size_t size,void*) {
    if (size!=sizeof(Draw)) return;
    Draw batch;
    std::memcpy(&batch,data,sizeof(batch));
    std::lock_guard lock(sGpuMutex);
    auto pipeline=pipeline_for(*ctx);
    if (!pipeline) {
        if (batch.count == 0) sPipelineWarmupQueued=false;
        return;
    }
    if (batch.count == 0) {
        sPipelineWarmed=true;
        sPipelineWarmupQueued=false;
        return;
    }
    wgpuRenderPassEncoderSetPipeline(ctx->pass,pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass,0,ctx->vertex_buffer,batch.vertices.offset,batch.vertices.size);
    wgpuRenderPassEncoderDraw(ctx->pass,batch.count,1,0,0);
}
bool drawable(daAlink_c* link,bool collection=false) {
    if (!link || link->checkWolf() || link->checkDeadHP() ||
        link->mClothesChangeWaitTimer!=0 || !link->mpLinkModel ||
        !link->mpLinkModel->getModelData() ||
        link->mpLinkModel->getModelData()->getJointNum()<=kJoints.back()) return false;
    if (collection) return link->checkStatusWindowDraw();
    return !link->checkStatusWindowDraw() && !link->checkPlayerNoDraw() &&
           !link->checkEventRun() && !link->checkSceneChangeAreaStart();
}
bool copy_pose(daAlink_c* link,unsigned joint,Pose& pose,bool presentation) {
    auto* matrix=link->mpLinkModel->getAnmMtx(joint);
    if (!matrix) return false;
    Mtx presented;
    if (presentation && sPresented && sPresented(matrix,presented)) matrix=presented;
    for (unsigned r=0;r<3;++r) for (unsigned c=0;c<4;++c) pose[r][c]=matrix[r][c];
    return finite(pose);
}
void discard(ModContext*,const GfxStageContext*,void*) { sHasPending=false; }
bool prepare_draw(daAlink_c* link,Animation& animation,const Mtx view,const Mtx44 projection,
                  Vec3 eye,bool presentation,Draw& batch) {
    const auto id=fopAcM_GetID(link);
    if (animation.owner!=id) { animation.reset(); animation.owner=id; }
    std::array<Pose,kJoints.size()> poses{};
    Pose root;
    if (!copy_pose(link,0,root,presentation)) { animation.reset(); return false; }
    for (unsigned i=0;i<kJoints.size();++i)
        if (!copy_pose(link,kJoints[i],poses[i],presentation)) { animation.reset(); return false; }
    const auto now=Clock::now();
    if (animation.last==Clock::time_point{}) animation.origin=animation.last=now;
    const float dt=std::chrono::duration<float>(now-animation.last).count();
    animation.last=now;
    animation.fade=std::min(1.0f,animation.fade+std::clamp(dt,0.0f,0.05f)*5);
    const float seconds=std::chrono::duration<float>(now-animation.origin).count();
    animation.motion.update(poses,origin(root),dt,seconds);
    build(poses,root,eye,animation.motion,seconds,animation.fade,sMesh);
    if (sMesh.empty()) return false;
    // Use the displayed form, not an unapplied setting changed in the menu.
    const Vec3 color{};
    GfxDeviceInfo device=GFX_DEVICE_INFO_INIT;
    if (svc_gfx->get_device_info(mod_ctx,&device)!=MOD_OK) return false;
    sVertices.clear(); sVertices.reserve(kVertexCount);
    for (const auto& v:sMesh) {
        GpuVertex vertex{};
        const float world[3]={v.world.x,v.world.y,v.world.z};
        float camera[3];
        for (unsigned r=0;r<3;++r) {
            camera[r]=view[r][3];
            for (unsigned c=0;c<3;++c) camera[r]+=view[r][c]*world[c];
        }
        for (unsigned r=0;r<4;++r)
            vertex.clip[r]=projection[r][0]*camera[0]+projection[r][1]*camera[1]+
                projection[r][2]*camera[2]+projection[r][3];
        // Match Aurora's conversion from GX [-w,0] clip depth to WebGPU.
        vertex.clip[2]=device.uses_reversed_z ? -vertex.clip[2]:vertex.clip[2]+vertex.clip[3];
        vertex.field[0]=v.field.x; vertex.field[1]=v.field.y; vertex.field[2]=v.field.z;
        vertex.opacity=v.opacity;
        vertex.color[0]=color.x; vertex.color[1]=color.y; vertex.color[2]=color.z;
        sVertices.push_back(vertex);
    }
    batch={{},static_cast<uint32_t>(sVertices.size())};
    return svc_gfx->push_verts(mod_ctx,sVertices.data(),sVertices.size()*sizeof(GpuVertex),
        alignof(GpuVertex),&batch.vertices)==MOD_OK;
}
void stage(ModContext*,const GfxStageContext* ctx,void*) {
    sHasPending=false;
    if (!sPipelineWarmed && !sPipelineWarmupQueued.exchange(true)) {
        Draw warmup{};
        if (svc_gfx->push_draw(mod_ctx,sDraw,&warmup,sizeof(warmup))!=MOD_OK) {
            sPipelineWarmupQueued=false;
        }
    }
    auto* link=daAlink_getAlinkActorClass();
    if (sOwner==fpcM_ERROR_PROCESS_ID_e || !ctx->game_view || !g_darkLinkVisualActive ||
        !drawable(link) || fopAcM_GetID(link)!=sOwner) {
        sWorld.reset(); return;
    }
    const auto& view=*static_cast<const view_class*>(ctx->game_view);
    sHasPending=prepare_draw(link,sWorld,view.viewMtx,view.projMtx,
        {view.lookat.eye.x,view.lookat.eye.y,view.lookat.eye.z},true,sPending);
}
HookAction before_collection(ModContext*,void*,void*,void*) {
    dark_link_eye_bloom::begin_collection();
    auto* link=daAlink_getAlinkActorClass();
    sCollectionDrawing=g_darkLinkVisualActive && drawable(link,true);
    if (!sCollectionDrawing) { sCollection.reset(); return HOOK_CONTINUE; }
    // Item3D has its own evaluated model pose, camera and viewport. Do not use
    // world interpolation replacements or let menu rotation create sprint lag.
    std::memcpy(sCollectionView,j3dSys.getViewMtx(),sizeof(sCollectionView));
    Mtx inverse;
    if (!MTXInverse(sCollectionView,inverse)) { sCollectionDrawing=false; return HOOK_CONTINUE; }
    sCollectionEye={inverse[0][3],inverse[1][3],inverse[2][3]};
    float p[7]; GXGetProjectionv(p);
    std::memset(sCollectionProjection,0,sizeof(sCollectionProjection));
    sCollectionProjection[0][0]=p[1]; sCollectionProjection[1][1]=p[3];
    sCollectionProjection[2][2]=p[5]; sCollectionProjection[2][3]=p[6];
    if (p[0]==0) {
        sCollectionProjection[0][2]=p[2]; sCollectionProjection[1][2]=p[4];
        sCollectionProjection[3][2]=-1;
    } else {
        sCollectionProjection[0][3]=p[2]; sCollectionProjection[1][3]=p[4];
        sCollectionProjection[3][3]=1;
    }
    return HOOK_CONTINUE;
}
void after_collection(ModContext*,void*,void*,void*) {
    if (sCollectionDrawing) {
        sCollectionDrawing=false;
        auto* link=daAlink_getAlinkActorClass();
        if (g_darkLinkVisualActive && drawable(link,true)) {
            Draw batch;
            if (prepare_draw(link,sCollection,sCollectionView,sCollectionProjection,sCollectionEye,false,batch))
                svc_gfx->push_draw(mod_ctx,sDraw,&batch,sizeof(batch));
        } else {
            sCollection.reset();
        }
    }
    dark_link_eye_bloom::end_collection();
}
void after_translucent(ModContext*,void*,void*,void*) {
    if (!sHasPending) return;
    sHasPending=false;
    // After world XLU/fog overlays, before post-processing and HUD. Occlusion
    // still uses scene depth; neither depth, alpha nor normal targets are written.
    svc_gfx->push_draw(mod_ctx,sDraw,&sPending,sizeof(sPending));
}
}
void stop_dark_link_veil() {
    sOwner=fpcM_ERROR_PROCESS_ID_e;
    sHasPending=false;
    sWorld.reset();
    sCollection.reset();
}
void update_dark_link_veil() {
    auto* link=daAlink_getAlinkActorClass();
    if (!g_darkLinkVisualActive || !drawable(link)) {
        stop_dark_link_veil();
        return;
    }
    const auto id=fopAcM_GetID(link);
    if (sOwner!=id) { stop_dark_link_veil(); sOwner=id; }
}
void initialize_dark_link_veil() {
    if (!svc_gfx || !svc_hook || sDraw) return;
    void* address=nullptr;
    if (svc_hook->resolve && svc_hook->resolve(mod_ctx,"dusk::interp::lookup_replacement",&address,nullptr)==MOD_OK)
        sPresented=reinterpret_cast<LookupPresentationMatrix>(address);
    const GfxDrawTypeDesc drawDesc={sizeof(GfxDrawTypeDesc),"Dark Link aura",draw,nullptr};
    const GfxStageHookDesc stageDesc={sizeof(GfxStageHookDesc),stage,nullptr};
    const GfxStageHookDesc discardDesc={sizeof(GfxStageHookDesc),discard,nullptr};
    if (svc_gfx->register_draw_type(mod_ctx,&drawDesc,&sDraw)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_SCENE_BEGIN,&discardDesc,&sBegin)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_SCENE_AFTER_OPAQUE,&stageDesc,&sStage)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_FRAME_BEFORE_HUD,&discardDesc,&sEnd)!=MOD_OK) {
        shutdown_dark_link_veil(); svc_log->warn(mod_ctx,"Dark Link aura: graphics registration failed"); return;
    }
    if (mods::hook::add_post<DarkVeilTranslucentHook>(svc_hook,after_translucent)!=MOD_OK) {
        mods::hook::uninstall<DarkVeilTranslucentHook>(svc_hook);
        shutdown_dark_link_veil(); svc_log->warn(mod_ctx,"Dark Link aura: translucent hook failed"); return;
    }
    sHook=true;
    if (mods::hook::add_pre<DarkVeilCollectionHook>(svc_hook,before_collection)!=MOD_OK ||
        mods::hook::add_post<DarkVeilCollectionHook>(svc_hook,after_collection)!=MOD_OK) {
        mods::hook::uninstall<DarkVeilCollectionHook>(svc_hook);
        shutdown_dark_link_veil(); svc_log->warn(mod_ctx,"Dark Link aura: Collection hook failed"); return;
    }
    sCollectionHook=true;
}
void shutdown_dark_link_veil() {
    stop_dark_link_veil();
    if (sHook) mods::hook::uninstall<DarkVeilTranslucentHook>(svc_hook);
    sHook=false;
    if (sCollectionHook) mods::hook::uninstall<DarkVeilCollectionHook>(svc_hook);
    sCollectionHook=sCollectionDrawing=false;
    if (sBegin) svc_gfx->unregister_stage_hook(mod_ctx,sBegin);
    if (sStage) svc_gfx->unregister_stage_hook(mod_ctx,sStage);
    if (sEnd) svc_gfx->unregister_stage_hook(mod_ctx,sEnd);
    if (sDraw) svc_gfx->unregister_draw_type(mod_ctx,sDraw);
    sBegin=sStage=sEnd=sDraw=0; sPresented=nullptr;
    sPipelineWarmed=false;
    sPipelineWarmupQueued=false;
    sMesh.clear(); sVertices.clear();
    std::lock_guard lock(sGpuMutex);
    for (const auto& p:sPipelines) wgpuRenderPipelineRelease(p.handle);
    sPipelines.clear();
    if (sShader) wgpuShaderModuleRelease(sShader);
    sShader=nullptr;
}
}
