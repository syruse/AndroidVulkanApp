#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragHSVFactors;
layout(location = 0) out vec4 outColor;

const float Epsilon = 1e-10;
vec3 RGBtoHSV(in vec3 RGB)
{
    vec4  P   = (RGB.g < RGB.b) ? vec4(RGB.bg, -1.0, 2.0/3.0) : vec4(RGB.gb, 0.0, -1.0/3.0);
    vec4  Q   = (RGB.r < P.x) ? vec4(P.xyw, RGB.r) : vec4(RGB.r, P.yzx);
    float C   = Q.x - min(Q.w, Q.y);
    float H   = abs((Q.w - Q.y) / (6.0 * C + Epsilon) + Q.z);
    vec3  HCV = vec3(H, C, Q.x);
    float S   = HCV.y / (HCV.z + Epsilon);
    return vec3(HCV.x, S, HCV.z);
}

vec3 HSVtoRGB(in vec3 HSV)
{
    float H   = HSV.x;
    float R   = abs(H * 6.0 - 3.0) - 1.0;
    float G   = 2.0 - abs(H * 6.0 - 2.0);
    float B   = 2.0 - abs(H * 6.0 - 4.0);
    vec3  RGB = clamp(vec3(R, G, B), 0.0, 1.0);
    return ((RGB - 1.0) * HSV.y + 1.0) * HSV.z;
}

void main() {
    vec4 color = texture(texSampler, fragTexCoord);

    // enabling filter for the right quad only
    if (fragHSVFactors.a > Epsilon) {
        vec3 color_hsv = RGBtoHSV(color.rgb);
        /* fragHSVFactors are factors in the range [0.0, 1.0].
           0.5 means that the image is kept as it is. if the factor is greater 0.5 than the image is saturated
           and if it is less than 0.5 the image is bleached */
        color_hsv.rgb *= (fragHSVFactors.rgb * 2.0);
        color.rgb = HSVtoRGB(color_hsv.rgb);
    }

    outColor = color;
}