#version 430 core
in  vec2 vUV;
out vec4 FragColor;
uniform sampler2D uAccum;   // rgba32f: rgb = sum, a = count

vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    // The compute shader writes pixels using gl_GlobalInvocationID.y, which
    // increases top-to-bottom (y=0 is top). OpenGL texture sampling puts v=0
    // at the bottom by default, so we flip v here to match the CPU renderer's
    // "row 0 is top of image" convention.
    vec2 uv = vec2(vUV.x, 1.0 - vUV.y);
    vec4 s = texture(uAccum, uv);
    vec3 col = (s.a > 0.0) ? s.rgb / s.a : vec3(0.0);
    col = aces(col);
    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(col, 1.0);
}
