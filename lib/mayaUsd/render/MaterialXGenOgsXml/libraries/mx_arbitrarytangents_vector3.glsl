void mx_arbitrarytangents_vector3(vec3 position, vec3 normal, out vec3 tangent)
{
    // Get screen space derivatives of position
    vec3 dPdx = dFdx(position);

    // Ensure position derivatives are perpendicular to normal
    tangent = normalize(dPdx - dot(dPdx, normal) * normal);
    
}
