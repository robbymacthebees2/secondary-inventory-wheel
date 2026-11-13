namespace oot_hearts {
namespace {

constexpr std::array<const char*, 5> kTextureFiles = {
    "res/oot_hearts/tex1_24x22_dd9e626bfda37aa9_2.dds",
    "res/oot_hearts/tex1_36x32_214a7429f6be3a2a_2.dds",
    "res/oot_hearts/tex1_36x32_80fb66461bf56402_2.dds",
    "res/oot_hearts/tex1_36x32_a300f0989bfb44c1_2.dds",
    "res/oot_hearts/tex1_36x32_e2ee16bf0e7f6f5b_2.dds",
};

std::array<TextureReplacementHandle, kTextureFiles.size()> s_handles{};
bool s_registered = false;
bool s_serviceWarningReported = false;

bool enabled() {
    return get_bool_option(g_ootStyleHeartsEnabled, false);
}

void unregister_all() {
    if (s_ootHeartsTextureService != nullptr) {
        for (auto& handle : s_handles) {
            if (handle != 0) {
                s_ootHeartsTextureService->unregister(mod_ctx, handle);
                handle = 0;
            }
        }
    }
    s_registered = false;
}

}

void update() {
    const bool want = enabled();
    if (want == s_registered) return;
    if (!want) {
        unregister_all();
        return;
    }
    if (s_ootHeartsTextureService == nullptr ||
        s_ootHeartsTextureService->register_file == nullptr)
    {
        if (!s_serviceWarningReported) {
            svc_log->warn(mod_ctx,
                "OOT Style Hearts enabled, but the texture service is unavailable");
            s_serviceWarningReported = true;
        }
        return;
    }

    s_serviceWarningReported = false;
    for (std::size_t i = 0; i < kTextureFiles.size(); ++i) {
        const ModResult result = s_ootHeartsTextureService->register_file(
            mod_ctx, kTextureFiles[i], &s_handles[i]);
        if (result != MOD_OK) {
            svc_log->error(mod_ctx, "failed to register an OOT heart texture");
            unregister_all();
            return;
        }
    }
    s_registered = true;
    svc_log->info(mod_ctx, "OOT Style Hearts textures enabled");
}

void shutdown() {
    unregister_all();
    s_serviceWarningReported = false;
}

}
