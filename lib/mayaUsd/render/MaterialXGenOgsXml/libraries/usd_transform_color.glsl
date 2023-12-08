vec3 usd_srgb_texture_to_lin_rec709(vec3 color)
{
    bvec3 isAbove = greaterThan(color, vec3(0.04045));
    vec3 linSeg = color / 12.92;
    vec3 powSeg = pow(max(color + vec3(0.055), vec3(0.0)) / 1.055, vec3(2.4));
    return mix(linSeg, powSeg, vec3(isAbove));
}
