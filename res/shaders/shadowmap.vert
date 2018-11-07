#version 400 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

struct light
{
    vec3 v3_lightPosition;
};

uniform light lt;
uniform mat4 m4_ModelViewProjection;

void main()
{

    float normalOffset = -50.0f;
    vec3 light_dir = normalize(lt.v3_lightPosition);
    float cosAngle = clamp(dot(light_dir, in_normal),0.0f,1.0f);
    vec3 scaledNormalOffset = in_normal * (normalOffset * cosAngle * 1.0f/2048.0f);

    gl_Position = m4_ModelViewProjection*vec4(scaledNormalOffset+in_position, 1.0);
}
