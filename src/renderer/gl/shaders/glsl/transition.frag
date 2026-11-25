#version 300 es

precision highp float;

in vec2 v_texcoord;

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float progress;
uniform float alpha;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 blended = mix(texture(tex1, v_texcoord), texture(tex2, v_texcoord), progress);
    fragColor = blended * alpha;
}
