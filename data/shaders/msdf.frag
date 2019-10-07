#extension GL_OES_standard_derivatives : enable
precision mediump float;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform sampler2D msdf;
uniform float pxRange;
uniform vec4 fgColor;
uniform float texelSize;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float sample_opacity(vec2 texcoord) {
    float distPerTexel = 1.0 / pxRange;
    float texelPerTexcoord = texelSize;
    float distPerTexcoord = distPerTexel * texelPerTexcoord;
    vec3 sample = texture2D(msdf, texcoord).rgb;
    float sigDist = median(sample.r, sample.g, sample.b);
    float texcoordPerPixel = length(vec2(length(dFdx(texcoord)), length(dFdy(texcoord)))) / sqrt(2.0);
    float distPerPixel = distPerTexcoord * texcoordPerPixel;
    float antialiasRange = distPerPixel / 2.0;
    float opacity = smoothstep(0.5 - antialiasRange, 0.5 + antialiasRange, sigDist);

    return opacity;
}

void main() {
    float opacity = sample_opacity(v_texcoord);

    gl_FragColor = vec4(fgColor.rgb, fgColor.a * opacity);
}
