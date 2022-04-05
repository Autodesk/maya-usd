void mx_texcoordtangents_vector3(vec3 position, vec3 normal, vec2 texcoord, out vec3 tangent)
{
    // Calculation of TBN matrix and terminology based on "Surface 
    // Gradient-Based Bump Mapping Framework" (2020)

    // Get screen space derivatives of position
    vec3 dPdx = dFdx(position);
    vec3 dPdy = dFdy(position);

    // Ensure position derivatives are perpendicular to normal
    vec3 sigmaX = dPdx - dot(dPdx, normal) * normal;
    vec3 sigmaY = dPdy - dot(dPdy, normal) * normal;

    float flipSign = dot(dPdy, cross(normal, dPdx)) < 0 ? -1 : 1;

    // Get screen space derivatives of st
    vec2 dSTdx = dFdx(texcoord);
    vec2 dSTdy = dFdy(texcoord);

    // Get determinant and determinant sign of st matrix
    float det = dot(dSTdx, vec2(dSTdy.y, -dSTdy.x));
    float signDet = det < 0 ? -1 : 1;

    // Get first column of inv st matrix
    // Don't divide by det, but scale by its sign
    vec2 invC0 = signDet * vec2(dSTdy.y, -dSTdx.y);

    vec3 T = sigmaX * invC0.x + sigmaY * invC0.y;

    if (abs(det) > 0) {
        T = normalize(T);
    }

    tangent = T;
}
