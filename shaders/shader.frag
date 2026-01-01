#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragVertPosition;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 fragTexCoord;

    if (abs(fragVertPosition.x) > abs(fragVertPosition.y) && abs(fragVertPosition.x) > abs(fragVertPosition.z))
        fragTexCoord = fragVertPosition.yz;
    else if (abs(fragVertPosition.y) > abs(fragVertPosition.x) && abs(fragVertPosition.y) > abs(fragVertPosition.z))
        fragTexCoord = fragVertPosition.xz;
    else fragTexCoord = fragVertPosition.xy;

    fragTexCoord += 0.5;

    outColor = vec4(4.0 * texture(texSampler, fragTexCoord).rgb, 1.0f);
}