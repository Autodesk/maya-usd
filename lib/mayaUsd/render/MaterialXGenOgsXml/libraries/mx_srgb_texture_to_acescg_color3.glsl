#include "stdlib/genglsl/lib/mx_transform_color.glsl"

void mx_srgb_texture_to_acescg_color3(vec3 _in, out vec3 result)
{
    result = mx_srgb_texture_to_lin_rec709(_in);

    // Same matrix as found in ../../vp2ShaderFragments/shaderFragments.cpp
    result = mat3(0.61309740, 0.07019372, 0.02061559,
                  0.33952315, 0.91635388, 0.10956977,
                  0.04737945, 0.01345240, 0.86981463) * result;
}
