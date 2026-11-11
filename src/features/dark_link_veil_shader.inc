// Fine rising ribbons: transparent outside each strand, no enclosing shell.
R"wgsl(
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) field: vec3f,
    @location(1) opacity: f32,
    @location(2) color: vec3f,
}
@vertex fn vs(@location(0) position: vec4f, @location(1) field: vec3f,
              @location(2) opacity: f32, @location(3) color: vec3f) -> VertexOutput {
    var out: VertexOutput;
    out.position=position; out.field=field; out.opacity=opacity; out.color=color;
    return out;
}
fn hash(p: vec3f) -> f32 {
    var q=fract(p*0.1031);
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
fn noise(p: vec3f) -> f32 {
    let cell=floor(p);
    let f=fract(p);
    let u=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(cell),hash(cell+vec3f(1,0,0)),u.x),
                   mix(hash(cell+vec3f(0,1,0)),hash(cell+vec3f(1,1,0)),u.x),u.y),
               mix(mix(hash(cell+vec3f(0,0,1)),hash(cell+vec3f(1,0,1)),u.x),
                   mix(hash(cell+vec3f(0,1,1)),hash(cell+vec3f(1,1,1)),u.x),u.y),u.z);
}
@fragment fn fs(input: VertexOutput) -> @location(0) vec4f {
    // X spans just one thin ribbon. Feather to exactly zero at both edges;
    // there is no low-opacity density floor filling the gaps between strands.
    let edge=pow(max(0.0,1.0-abs(input.field.x)),1.4);
    let flow=input.field.y;
    let seed=input.field.z;
    let detail=noise(vec3f(flow*0.65,seed*1.71,0.3));
    let alpha=0.72*input.opacity*edge*(0.58+0.42*detail);
    return vec4f(input.color,alpha);
}
)wgsl"
