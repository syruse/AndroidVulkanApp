#version 450

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragHSVFactors;// last component indicates whether filter is enabled\disabled
// Currently MVP containing rotation matix
layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
} ubo;

layout(push_constant) uniform constants
{
    vec3 hsv_factors;
} PushConstants;

vec2 positions[6] = vec2[](
vec2(-0.9, -0.5),
vec2(-0.1, -0.5),
vec2(-0.1, 0.5),
vec2(-0.1, 0.5),
vec2(-0.9, 0.5),
vec2(-0.9, -0.5)
);

vec2 tex_coords[6] = vec2[](
vec2(0.0f, 0.0f),
vec2(1.0f, 0.0f),
vec2(1.0f, 1.0f),
vec2(1.0f, 1.0f),
vec2(0.0f, 1.0f),
vec2(0.0f, 0.0f)
);

void main() {
    const int index = gl_VertexIndex % 6;
    const bool isRightQuad = gl_VertexIndex >= 6 ? true : false;
    vec2 pos = positions[index];
    fragHSVFactors = vec4(PushConstants.hsv_factors, 0.0);
    if (isRightQuad) {
        pos.x = 1.0 + pos.x;
        fragHSVFactors.a = 1.0;// enabling filter for the second quad only
    }
    gl_Position = ubo.MVP * vec4(pos, 0.0, 1.0);
    fragTexCoord = tex_coords[index];
}