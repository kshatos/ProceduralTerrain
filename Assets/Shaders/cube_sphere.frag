#version 330 core
const float PI = 3.14159265359;

//////////////////////////////
// CAMERA DATA
//////////////////////////////
uniform vec3 u_viewPos;


//////////////////////////////
// LIGHTING DATA
//////////////////////////////
struct PointLight
{
    vec3 position;
    float radiantFlux;
    float range;
    vec3 color;
};

struct DirectionalLight
{
    vec3 direction;
    float irradiance;
    vec3 color;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutoffAngle;
    float falloffRatio;
    float radiantIntensity;
    float range;
    vec3 color;
};

uniform float u_ambientRadiance;

#define MAX_POINT_LIGHTS 8
uniform int u_nPointLights;
uniform PointLight u_pointLights[MAX_POINT_LIGHTS];

#define MAX_DIRECTIONAL_LIGHTS 8
uniform int u_nDirectionalLights;
uniform DirectionalLight u_directionalLights[MAX_DIRECTIONAL_LIGHTS];

#define MAX_SPOT_LIGHTS 8
uniform int u_nSpotLights;
uniform SpotLight u_spotLights[MAX_SPOT_LIGHTS];

uniform mat4 u_lightTransform;
uniform sampler2D u_shadowBufferTexture;

//////////////////////////////
// PBR SURFACE MODEL
//////////////////////////////
struct PBRSurfaceData
{
    vec3 position;
    vec3 normal;
    vec3 albedo;
    float metallic;
    float roughness;
};

/* Trowbridge-Reitz GGX model */
float NormalDistribution_GGX(float cosNH, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float denominator = cosNH * cosNH * (roughnessSquared - 1.0) + 1.0;
    return roughnessSquared / (PI * denominator * denominator);
}

/* Height Correlated Smith View */
float View_HCSmith(float cosNL, float cosNV, float roughness)
{
    float Vv = cosNL * (cosNV * (1.0 - roughness) + roughness);
    float Vl = cosNV * (cosNL * (1.0 - roughness) + roughness);

    return 0.5 / (Vv + Vl);
}

/* Schlick approximation */
vec3 Fresnel_Schlick(float cosVH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosVH, 5.0);
}

vec3 BRDF(
    vec3 viewDir,
    vec3 lightDir,
    vec3 halfwayDir,
    PBRSurfaceData surface)
{
    float cosNH = clamp(dot(surface.normal, halfwayDir), 1.0e-6, 1.0);
    float cosNL = clamp(dot(surface.normal, lightDir), 1.0e-6, 1.0);
    float cosNV = clamp(dot(surface.normal, viewDir), 1.0e-6, 1.0);
    float cosVH = clamp(dot(viewDir, halfwayDir), 1.0e-6, 1.0);

    vec3 F0 = mix(vec3(0.04), surface.albedo, surface.metallic);
    vec3 diffuseColor =  (1.0 - surface.metallic) * surface.albedo;

    float D = NormalDistribution_GGX(cosNH, surface.roughness);
    float V = View_HCSmith(cosNL, cosNV, surface.roughness);
    vec3 F = Fresnel_Schlick(cosVH, F0);

    vec3 specular = D * V * F;
    vec3 diffuse = (vec3(1.0) - F) * diffuseColor / PI;

    vec3 f = diffuse + specular;

    return f * cosNL;
}

//////////////////////////////
// LIGHTING MODEL
//////////////////////////////
float PunctualLightAttenuation(float separation, float range)
{
    float separation2 = separation * separation;
    float relativeRange = min(1.0, separation / range);
    float relativeRange4 = relativeRange * relativeRange * relativeRange * relativeRange;

    return 1.0 / (separation2 + 1.0e-6) * (1.0 - relativeRange4);
}

float SpotLightAttenuation(
    SpotLight light,
    vec3 lightDir)
{
    float cosDL = dot(-light.direction, lightDir);
    float cosTMin = cos(light.cutoffAngle * (1.0 - light.falloffRatio));
    float cosTMax = cos(light.cutoffAngle);
    
    if (cosDL >= cosTMin)
        return 1.0;
    if (cosDL <= cosTMax)
        return 0.0;
    return smoothstep(0.0, 1.0, (cosDL - cosTMax) / (cosTMin - cosTMax));
}

vec3 PointLightReflectedRadiance(
    PointLight light,
    PBRSurfaceData surface)
{
    float separation = length(light.position - surface.position);
    vec3 viewDir = normalize(u_viewPos - surface.position);
    vec3 lightDir = normalize(light.position - surface.position);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec3 f = BRDF(viewDir, lightDir, halfwayDir, surface);
    vec3 L0 = light.color * light.radiantFlux / (4.0 * PI);
    float A = PunctualLightAttenuation(separation, light.range);

    return L0 * A * f;
}

vec3 DirectionalLightReflectedRadiance(
    DirectionalLight light,
    PBRSurfaceData surface)
{
    vec3 viewDir = normalize(u_viewPos - surface.position);
    vec3 lightDir = -normalize(light.direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec3 f = BRDF(viewDir, lightDir, halfwayDir, surface);
    vec3 L0 = light.color * light.irradiance ;

    return f * L0;
}

vec3 SpotLightReflectedRadiance(
    SpotLight light,
    PBRSurfaceData surface)
{
    float separation = length(light.position - surface.position);
    vec3 viewDir = normalize(u_viewPos - surface.position);
    vec3 lightDir = normalize(light.position - surface.position);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec3 f = BRDF(viewDir, lightDir, halfwayDir, surface);
    vec3 L0 = light.color * light.radiantIntensity / (4.0 * PI);
    float A = PunctualLightAttenuation(separation, light.range);
    float l = SpotLightAttenuation(light, lightDir);

    return L0 * A * l * f;
}

float DirectionalLightShadow(vec3 pos, vec3 normalDir, vec3 lightDir)
{
    vec4 lightSpacePos = u_lightTransform * vec4(pos, 1.0);

    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(u_shadowBufferTexture, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.001 * (1.0 - dot(normalDir, lightDir)), 0.002);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_shadowBufferTexture, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_shadowBufferTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias < pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}


//////////////////////////////
// MATERIAL DATA
//////////////////////////////
uniform samplerCube u_heightmap;
uniform samplerCube u_normal;
uniform samplerCube u_splatmap;
uniform sampler2D u_water_normalmap;
uniform sampler2D u_terrain_textures[4];

uniform float u_terrain_texture_scales[4];

in vec3 Pos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;
in vec2 TexCoord;
out vec4 FragColor;


//////////////////////////////
// SPLAT MAPPING
//////////////////////////////
/*
Taken from the paper
"Procedural Stochastic Textures by Tiling and Blending"
(Thomas Deliot and Eric Heitz)
*/
void TriangleGrid(
    vec2 uv,
    out float w1, out float w2, out float w3,
    out ivec2 vertex1, out ivec2 vertex2, out ivec2 vertex3)
{
    // Scaling of the input
    uv *= 3.464; // 2 * sqrt(3)

    // Skew input space into simplex triangle grid
    const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
    vec2 skewedCoord = gridToSkewedGrid * uv;

    // Compute local triangle vertex IDs and local barycentric coordinates
    ivec2 baseId = ivec2(floor(skewedCoord));
    vec3 temp = vec3(fract(skewedCoord), 0);
    temp.z = 1.0 - temp.x - temp.y;
    if (temp.z > 0.0)
    {
        w1 = temp.z;
        w2 = temp.y;
        w3 = temp.x;
        vertex1 = baseId;
        vertex2 = baseId + ivec2(0, 1);
        vertex3 = baseId + ivec2(1, 0);
    }
    else
    {
        w1 = -temp.z;
        w2 = 1.0 - temp.y;
        w3 = 1.0 - temp.x;
        vertex1 = baseId + ivec2(1, 1);
        vertex2 = baseId + ivec2(1, 0);
        vertex3 = baseId + ivec2(0, 1);
    }
}

vec2 StochasticTilingHash(vec2 x)
{
    return fract(sin((x) * mat2(127.1, 311.7, 269.5, 183.3) )*43758.5453);
}

vec4 SampleStochastic(sampler2D tex, vec2 uv)
{
    // Get triangle info
    float w1, w2, w3;
    ivec2 vertex1, vertex2, vertex3;
    TriangleGrid(uv, w1, w2, w3, vertex1, vertex2, vertex3);
        
    // Assign random offset to each triangle vertex
    vec2 uv1 = uv + StochasticTilingHash(vertex1);
    vec2 uv2 = uv + StochasticTilingHash(vertex2);
    vec2 uv3 = uv + StochasticTilingHash(vertex3);

    // Precompute UV derivatives 
    vec2 duvdx = dFdx(uv);
    vec2 duvdy = dFdy(uv);

    // Fetch Gaussian input
    vec4 G1 = textureGrad(tex, uv1, duvdx, duvdy);
    vec4 G2 = textureGrad(tex, uv2, duvdx, duvdy);
    vec4 G3 = textureGrad(tex, uv3, duvdx, duvdy);

    // Linear blending
    vec4 color = (
        w1 * G1 +
        w2 * G2 +
        w3 * G3);

    return color;
}

vec4 SampleTriplanar(
    sampler2D tex,
    vec2 xy,
    vec2 yz,
    vec2 zx,
    vec3 normal)
{
    vec4 xy_sample = pow(SampleStochastic(tex, xy), vec4(2.2));
    vec4 yz_sample = pow(SampleStochastic(tex, yz), vec4(2.2));
    vec4 zx_sample = pow(SampleStochastic(tex, zx), vec4(2.2));
    
    vec3 weights = pow(abs(normal), vec3(2.0));
    weights /= (weights.x + weights.y + weights.z);

    return (
        weights.x * yz_sample +
        weights.y * zx_sample +
        weights.z * xy_sample);
}

vec3 SampleTriplanarNormal(
    sampler2D tex,
    vec2 xy,
    vec2 yz,
    vec2 zx,
    vec3 normal)
{
    vec3 x_normal = 2.0 * SampleStochastic(tex, yz).xyz - 1.0;
    vec3 y_normal = 2.0 * SampleStochastic(tex, zx).xyz - 1.0;
    vec3 z_normal = 2.0 * SampleStochastic(tex, xy).xyz - 1.0;

    x_normal.z *= sign(normal.x);
    y_normal.z *= sign(normal.y);
    z_normal.z *= sign(normal.z);

    x_normal = vec3(x_normal.xy + normal.zy, abs(x_normal.z) * normal.x);
    y_normal = vec3(y_normal.xy + normal.xz, abs(y_normal.z) * normal.y);
    z_normal = vec3(z_normal.xy + normal.xy, abs(z_normal.z) * normal.z);

    vec3 weights = pow(abs(normal), vec3(2.0));
    weights /= (weights.x + weights.y + weights.z);

    return normalize(
        weights.x * x_normal.zyx +
        weights.y * y_normal.xzy +
        weights.z * z_normal.xyz);
}

vec4 SampleTerrain(vec3 position, vec3 normal)
{
    vec4 splat_weights = texture(u_splatmap, Pos);

    vec2 xy = Pos.xy;
    vec2 yz = Pos.yz;
    vec2 zx = Pos.zx;

    float scale = u_terrain_texture_scales[0];
    vec4 sample0 = SampleTriplanar(
        u_terrain_textures[0], scale*xy, scale*yz, scale*zx, normal);

    scale = u_terrain_texture_scales[1];
    vec4 sample1 = SampleTriplanar(
        u_terrain_textures[1], scale*xy, scale*yz, scale*zx, normal);
        
    scale = u_terrain_texture_scales[2];
    vec4 sample2 = SampleTriplanar(
        u_terrain_textures[2], scale*xy, scale*yz, scale*zx, normal);
    
    scale = u_terrain_texture_scales[3];
    vec4 sample3 = SampleTriplanar(
        u_terrain_textures[3], scale*xy, scale*yz, scale*zx, normal);

    return (
        sample0 * splat_weights.x +
        sample1 * splat_weights.y +
        sample2 * splat_weights.z +
        sample3 * splat_weights.w);
}


//////////////////////////////
// MAIN
//////////////////////////////
void main()
{
    vec3 water_normal = SampleTriplanarNormal(
        u_water_normalmap, 2*Pos.xy, 2*Pos.yz, 2*Pos.zx, Normal);
    vec3 land_normal = normalize(2.0 * texture(u_normal, Pos).xyz - 1.0);

    float height = texture(u_heightmap, Pos).x;
    vec3 normal = true ? water_normal : land_normal;
    vec3 albedo = true ? vec3(0.0f, 0.0f, 0.7f) : SampleTerrain(Pos, Normal).xyz;
    float roughness = true ? 0.05 : 0.80;

    PBRSurfaceData surface;
    surface.position = Pos;
    surface.normal = normal;
    surface.albedo = albedo;
    surface.metallic = 0.0;
    surface.roughness = roughness;
    surface.roughness *= surface.roughness;

    // Accumulate lighting contributions
    vec3 result = vec3(0.0f);
    for (int i = 0; i < u_nPointLights; i++)
    {
        result += PointLightReflectedRadiance(u_pointLights[i], surface);
    }
    for (int i=0; i < u_nDirectionalLights; i++)
    {
        float shadow = i==0 ? DirectionalLightShadow(surface.position, surface.normal, -u_directionalLights[i].direction) : 1.0;
        result += shadow * DirectionalLightReflectedRadiance(u_directionalLights[i], surface);
    }
    for (int i = 0; i < u_nSpotLights; i++)
    {
        result += SpotLightReflectedRadiance(u_spotLights[i], surface);
    }

    result += vec3(u_ambientRadiance) * surface.albedo;
    
    // HDR Tonemap and gamma correction
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2)); 


    FragColor = vec4(result, 1.0);
}