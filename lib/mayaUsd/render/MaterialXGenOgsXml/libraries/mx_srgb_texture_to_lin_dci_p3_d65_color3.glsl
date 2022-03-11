#include "stdlib/genglsl/lib/mx_transform_color.glsl"

void mx_srgb_texture_to_lin_dci_p3_d65_color3(vec3 _in, out vec3 result)
{
    result = mx_srgb_texture_to_lin_rec709(_in);

    // Same matrix as found in ../../vp2ShaderFragments/shaderFragments.cpp
    result = mat3(0.82246197, 0.03319420, 0.01708263,
                  0.17753803, 0.96680580, 0.07239744,
                  0.,         0.,         0.91051993) * result;
}
