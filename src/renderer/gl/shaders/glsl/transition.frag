#version 300 es

precision highp float;

in vec2 v_texcoord;

uniform sampler2D tex1;      // Start frame (fit pre-resolved in C++ via renderFitFrame)
uniform sampler2D tex2;      // End frame (fit pre-resolved in C++)
uniform float progress;      // 0.0 = fully start, 1.0 = fully end
uniform float alpha;         // Overall opacity

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 blended = mix(texture(tex1, v_texcoord), texture(tex2, v_texcoord), progress);
    fragColor = blended * alpha;
}
