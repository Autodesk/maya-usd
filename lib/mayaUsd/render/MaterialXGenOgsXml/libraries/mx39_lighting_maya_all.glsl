#ifdef MAYA_MX39_USING_ENVIRONMENT_FIS

vec3 mx39_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, Mx39FresnelData fd)
{
    if (mayaGetSpecularEnvironmentNumLOD() == 0) {
        return vec3(0);
    }
    
    // Generate tangent frame.
    X = normalize(X - dot(X, N) * N);
    vec3 Y = cross(N, X);
    mat3 tangentToWorld = mat3(X, Y, N);

    // Transform the view vector to tangent space.
    V = vec3(dot(V, X), dot(V, Y), dot(V, N));

    // Compute derived properties.
    float NdotV = clamp(V.z, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(alpha);
    float G1V = mx_ggx_smith_G1(NdotV, avgAlpha);
    
    // Integrate outgoing radiance using filtered importance sampling.
    // http://cgg.mff.cuni.cz/~jaroslav/papers/2008-egsr-fis/2008-egsr-fis-final-embedded.pdf
    vec3 radiance = vec3(0.0);
    for (int i = 0; i < MX_NUM_FIS_SAMPLES; i++)
    {
        vec2 Xi = mx_spherical_fibonacci(i, MX_NUM_FIS_SAMPLES);

        // Compute the half vector and incoming light direction.
        vec3 H = mx_ggx_importance_sample_VNDF(Xi, V, alpha);
        vec3 L = fd.refraction ? mx_refraction_solid_sphere(-V, H, fd.ior.x) : -reflect(V, H);
        
        // Compute dot products for this sample.
        float NdotL = clamp(L.z, M_FLOAT_EPS, 1.0);
        float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

        // Sample the environment light from the given direction.
        vec3 Lw = tangentToWorld * L;
        float pdf = mx_ggx_NDF(H, alpha) * G1V / (4.0 * NdotV);
        float lod = mx_latlong_compute_lod_adsk(Lw, pdf, float(mayaGetSpecularEnvironmentNumLOD() - 1), MX_NUM_FIS_SAMPLES);
        vec3 sampleColor = mayaSampleSpecularEnvironmentAtLOD(Lw, lod);

        // Compute the Fresnel term.
        vec3 F = mx39_compute_fresnel(VdotH, fd);

        // Compute the geometric term.
        float G = mx_ggx_smith_G2(NdotL, NdotV, avgAlpha);

        // Compute the combined FG term, which is inverted for refraction.
        vec3 FG = fd.refraction ? vec3(1.0) - (F * G) : F * G;

        // Add the radiance contribution of this sample.
        // From https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
        //   incidentLight = sampleColor * NdotL
        //   microfacetSpecular = D * F * G / (4 * NdotL * NdotV)
        //   pdf = D * G1V / (4 * NdotV);
        //   radiance = incidentLight * microfacetSpecular / pdf
        radiance += sampleColor * FG;
    }

    // Apply the global component of the geometric term and normalize.
    radiance /= G1V * float(MX_NUM_FIS_SAMPLES);

    // Return the final radiance.
    return radiance;
}

#endif

#ifdef MAYA_MX39_USING_ENVIRONMENT_PREFILTER_V1

vec3 mx39_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, Mx39FresnelData fd)
{
    N = mx_forward_facing_normal(N, V);
    vec3 L = fd.refraction ? mx_refraction_solid_sphere(-V, N, fd.ior.x) : -reflect(V, N);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(alpha);
    vec3 F = mx39_compute_fresnel(NdotV, fd);
    float G = mx_ggx_smith_G2(NdotV, NdotV, avgAlpha);
    vec3 FG = fd.refraction ? vec3(1.0) - (F * G) : F * G;
    vec3 Li = mix(g_specularI, g_diffuseI, avgAlpha);
    return Li * FG;
}

#endif

#ifdef MAYA_MX39_USING_ENVIRONMENT_PREFILTER_V2

vec3 mx39_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, Mx39FresnelData fd)
{
    N = mx_forward_facing_normal(N, V);
    vec3 L = fd.refraction ? mx_refraction_solid_sphere(-V, N, fd.ior.x) : -reflect(V, N);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(alpha);
    vec3 F = mx39_compute_fresnel(NdotV, fd);
    float G = mx_ggx_smith_G2(NdotV, NdotV, avgAlpha);
    vec3 FG = fd.refraction ? vec3(1.0) - (F * G) : F * G;
    float phongExp = mayaRoughnessToPhongExp(sqrt(avgAlpha));
    vec3 Li = mayaGetSpecularEnvironment(N, V, phongExp);
    return Li * FG;
}

#endif

#ifdef MAYA_MX39_USING_ENVIRONMENT_NONE

vec3 mx39_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, Mx39FresnelData fd)
{
    return vec3(0.0);
}

#endif
