#include "stdlib/genglsl/lib/mx_transform_color.glsl"

void mx_srgb_texture_to_linrec2020_color4(vec4 _in, out vec4 result)
{
    vec3 color = mx_srgb_texture_to_lin_rec709(_in.rgb);

    // Same matrix as found in ../../vp2ShaderFragments/shaderFragments.cpp
    color = mat3(0.62740389, 0.06909729, 0.01639144,
                 0.32928304, 0.91954039, 0.08801331,
                 0.04331307, 0.01136232, 0.89559525) * color;

    result = vec4(color, _in.a);
}
