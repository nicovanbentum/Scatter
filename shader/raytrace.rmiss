#version 460

#extension GL_NV_ray_tracing : require

struct Payload {
    vec3 value;
};

layout(location = 0) rayPayloadInNV Payload payload;

void main() {
    payload.value = vec3(1.0);
}