#include "libraries/stdlib/genglsl/lib/mx_transform_color.glsl"

void mx_srgb_texture_to_aces_2065_1_color3(vec3 _in, out vec3 result)
{
    result = mx_srgb_texture_to_lin_rec709(_in);

    // Same matrix as found in ../../vp2ShaderFragments/shaderFragments.cpp
    result = mat3(0.43963298, 0.08977644, 0.01754117,
                  0.38298870, 0.81343943, 0.11154655,
                  0.17737832, 0.09678413, 0.87091228) * result;
}
