#include "libraries/stdlib/genglsl/lib/mx_transform_color.glsl"

void mx_srgb_texture_to_acescg_color4(vec4 _in, out vec4 result)
{
    vec3 color = mx_srgb_texture_to_lin_rec709(_in.rgb);

    // Same matrix as found in ../../vp2ShaderFragments/shaderFragments.cpp
    color = mat3(0.61309740, 0.07019372, 0.02061559,
                 0.33952315, 0.91635388, 0.10956977,
                 0.04737945, 0.01345240, 0.86981463) * color;

    result = vec4(color, _in.a);
}
