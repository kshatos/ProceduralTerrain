#version 330 core

//////////////////////////////
// CAMERA DATA
//////////////////////////////
uniform mat3 u_NormalMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

//////////////////////////////
// OBJECT DATA
//////////////////////////////
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec2 aTexCoord;

out vec3 Pos;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;
out vec2 TexCoord;


void main()
{
    Pos = vec3(u_ModelMatrix * vec4(aPos, 1.0));
    Normal = u_NormalMatrix * aNormal;
    Tangent = u_NormalMatrix * aTangent;
    Bitangent = u_NormalMatrix * aBitangent;
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * u_ModelMatrix * vec4(aPos, 1.0);
}