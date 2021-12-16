#version 330 core
out vec4 FragColor;

in vec3 Pos;
in vec3 Normal;
in vec2 TexCoord;

uniform samplerCube u_albedo;

void main()
{
    FragColor = texture(u_albedo, Pos);
}