R"wgsl(
@group(0) @binding(0) var eyeColor: texture_2d<f32>;
@group(0) @binding(1) var eyeDepth: texture_2d<f32>;
@group(0) @binding(2) var sceneDepth: texture_2d<f32>;
@group(0) @binding(3) var extracted: texture_storage_2d<rgba16float, write>;
override reversedZ: bool = false;

@compute @workgroup_size(8, 8)
fn extract(@builtin(global_invocation_id) id: vec3u) {
    if (any(id.xy >= textureDimensions(extracted))) { return; }
    var sum = vec3f(0.0);
    let dims = vec2i(textureDimensions(eyeColor));
    for (var y = 0; y < 2; y++) {
        for (var x = 0; x < 2; x++) {
            let p = vec2i(id.xy * 2u) + vec2i(x, y);
            if (any(p >= dims)) { continue; }
            let eye = textureLoad(eyeColor, p, 0);
            let z = textureLoad(eyeDepth, p, 0).r;
            let scene = textureLoad(sceneDepth, p, 0).r;
            let visible = select(z <= scene + 0.000002, z >= scene - 0.000002, reversedZ);
            if (visible) { sum += eye.rgb; }
        }
    }
    textureStore(extracted, vec2i(id.xy), vec4f(sum * 0.25, 0.0));
}

@group(0) @binding(4) var blurSource: texture_2d<f32>;
@group(0) @binding(5) var blurred: texture_storage_2d<rgba16float, write>;
const weights = array<f32, 5>(0.22702703, 0.19459459, 0.12162162, 0.05405405, 0.01621622);

@compute @workgroup_size(8, 8)
fn horizontal(@builtin(global_invocation_id) id: vec3u) {
    let dims = vec2i(textureDimensions(blurSource));
    let p = vec2i(id.xy);
    if (any(p >= dims)) { return; }
    var sum = textureLoad(blurSource, p, 0).rgb * weights[0];
    for (var i = 1; i <= 4; i++) {
        let a = p - vec2i(i, 0);
        let b = p + vec2i(i, 0);
        if (a.x >= 0) { sum += textureLoad(blurSource, a, 0).rgb * weights[i]; }
        if (b.x < dims.x) { sum += textureLoad(blurSource, b, 0).rgb * weights[i]; }
    }
    textureStore(blurred, p, vec4f(sum, 0.0));
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}
@vertex
fn vs(@builtin(vertex_index) index: u32) -> VertexOutput {
    let p = vec2f(f32((index << 1u) & 2u), f32(index & 2u));
    var output: VertexOutput;
    output.position = vec4f(p * 2.0 - 1.0, 0.0, 1.0);
    output.uv = vec2f(p.x, 1.0 - p.y);
    return output;
}
@group(0) @binding(6) var glow: texture_2d<f32>;
@group(0) @binding(7) var linearSampler: sampler;
@group(0) @binding(8) var<uniform> bloomParameters: vec4f;
fn sampleGlow(uv: vec2f) -> vec3f {
    if (any(uv < vec2f(0.0)) || any(uv > vec2f(1.0))) { return vec3f(0.0); }
    return textureSampleLevel(glow, linearSampler, uv, 0.0).rgb;
}
@fragment
fn fs(input: VertexOutput) -> @location(0) vec4f {
    let step = 1.0 / f32(textureDimensions(glow).y);
    var sum = sampleGlow(input.uv) * weights[0];
    for (var i = 1; i <= 4; i++) {
        let offset = vec2f(0.0, f32(i) * step);
        sum += (sampleGlow(input.uv - offset) + sampleGlow(input.uv + offset)) * weights[i];
    }
    return vec4f(sum * (1.5 * bloomParameters.x), 0.0);
}
)wgsl"
