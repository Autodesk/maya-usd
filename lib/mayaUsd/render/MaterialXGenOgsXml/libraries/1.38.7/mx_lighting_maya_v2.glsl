// This is the same algorithm as found in libraries\pbrlib\genglsl\lib\mx_environment_prefilter.glsl
// but adjusted for Maya.
//
// Since we are on a more recent versions of Maya, we have external lighting functions that can be
// called to fetch environment samples:

#include "libraries/pbrlib/genglsl/lib/mx_microfacet_specular.glsl"

vec3 mx_environment_irradiance(vec3 N)
{
    return mayaGetIrradianceEnvironment(N);
}

vec3 mx_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, FresnelData fd)
{
    N = mx_forward_facing_normal(N, V);
    vec3 L = reflect(-V, N);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(alpha);
    vec3 F = mx_compute_fresnel(NdotV, fd);
    float G = mx_ggx_smith_G2(NdotV, NdotV, avgAlpha);
    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);
    float phongExp = mayaRoughnessToPhongExp(sqrt(avgAlpha));
    vec3 Li = mayaGetSpecularEnvironment(N, V, phongExp);

    return Li * F * G * comp;
}
