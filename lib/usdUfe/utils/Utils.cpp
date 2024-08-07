//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Utils.h"

#include <pxr/base/tf/stringUtils.h>

#include <cctype>
#include <regex>
#include <string>
#include <unordered_map>

namespace USDUFE_NS_DEF {

namespace {

// This is an ugly and temporary solution to the lack of uiname metadata at the NodeDef level in
// MaterialX. There is a PR in flight to add the needed uinames to the official MaterialX repository
// (PR 1959), but it might take some time before it reaches MayaUSD via an updated USD build.
std::string _getMaterialXUIName(const std::string& nodename)
{
    static const auto kUINames = std::unordered_map<std::string, std::string> {
        { "LamaSSS", "Lama Subsurface Scattering" },
        { "UsdPreviewSurface", "USD Preview Surface" },
        { "UsdPrimvarReader", "USD Primvar Reader" },
        { "UsdTransform2d", "USD Transform 2D" },
        { "UsdUVTexture", "USD UV Texture" },
        { "absorption_vdf", "Absorption VDF" },
        { "absval", "Absolute Value" },
        { "acescg_to_lin_rec709", "ACEScg to Linear Rec. 709" },
        { "adobergb_to_lin_rec709", "Adobe RGB to Linear Rec. 709" },
        { "ambientocclusion", "Ambient Occlusion" },
        { "anisotropic_vdf", "Anisotropic VDF" },
        { "arrayappend", "Array Append" },
        { "artistic_ior", "Artistic IOR" },
        { "burley_diffuse_bsdf", "Burley Diffuse BSDF" },
        { "cellnoise2d", "2D Cellular Noise" },
        { "cellnoise3d", "3D Cellular Noise" },
        { "colorcorrect", "Color Correct" },
        { "conductor_bsdf", "Conductor BSDF" },
        { "conical_edf", "Conical EDF" },
        { "creatematrix", "Create Matrix" },
        { "crossproduct", "Cross Product" },
        { "curveadjust", "Curve Adjust" },
        { "dielectric_bsdf", "Dielectric BSDF" },
        { "disjointover", "Disjoint Over" },
        { "disney_brdf_2012", "Disney BRDF 2012" },
        { "disney_bsdf_2015", "Disney BSDF 2015" },
        { "dotproduct", "Dot Product" },
        { "facingratio", "Facing Ratio" },
        { "fractal3d", "3D Fractal Noise" },
        { "g18_rec709_to_lin_rec709", "Gamma 1.8 Rec. 709 to Linear Rec. 709" },
        { "g22_ap1_to_lin_rec709", "Gamma 2.2 AP1 to Linear Rec. 709" },
        { "g22_rec709_to_lin_rec709", "Gamma 2.2 Rec. 709 to Linear Rec. 709" },
        { "generalized_schlick_bsdf", "Generalized Schlick BSDF" },
        { "generalized_schlick_edf", "Generalized Schlick EDF" },
        { "geomcolor", "Geometric Color" },
        { "geompropvalue", "Geometric Property Value" },
        { "gltf_colorimage", "glTF Color Image" },
        { "gltf_image", "glTF Image" },
        { "gltf_iridescence_thickness", "glTF Iridescence Thickness" },
        { "gltf_normalmap", "glTF Normal Map" },
        { "gltf_pbr", "glTF PBR" },
        { "heighttonormal", "Height to Normal" },
        { "hsvadjust", "HSV Adjust" },
        { "hsvtorgb", "HSV to RGB" },
        { "ifequal", "If Equal" },
        { "ifgreater", "If Greater" },
        { "ifgreatereq", "If Greater or Equal" },
        { "invertmatrix", "Invert Matrix" },
        { "lin_adobergb_to_lin_rec709", "Linear Adobe RGB to Linear Rec. 709" },
        { "lin_displayp3_to_lin_rec709", "Linear Display P3 to Linear Rec. 709" },
        { "measured_edf", "Measured EDF" },
        { "noise2d", "2D Perlin Noise" },
        { "noise3d", "3D Perlin Noise" },
        { "normalmap", "Normal Map" },
        { "open_pbr_anisotropy", "OpenPBR Anisotropy" },
        { "open_pbr_surface", "OpenPBR Surface" },
        { "open_pbr_surface_to_standard_surface", "OpenPBR Surface to Standard Surface" },
        { "oren_nayar_diffuse_bsdf", "Oren-Nayar Diffuse BSDF" },
        { "place2d", "Place 2D" },
        { "premult", "Premultiply" },
        { "ramp4", "4-corner Bilinear Value Ramp" },
        { "ramplr", "Left-to-right Bilinear Value Ramp" },
        { "ramptb", "Top-to-bottom Bilinear Value Ramp" },
        { "randomcolor", "Random Color" },
        { "randomfloat", "Random Float" },
        { "rec709_display_to_lin_rec709", "Rec. 709 Display to Linear Rec. 709" },
        { "rgbtohsv", "RGB to HSV" },
        { "rotate2d", "Rotate 2D" },
        { "rotate3d", "Rotate 3D" },
        { "safepower", "Safe Power" },
        { "sheen_bsdf", "Sheen BSDF" },
        { "smoothstep", "Smooth Step" },
        { "splitlr", "Left-right Split Matte" },
        { "splittb", "Top-bottom Split Matte" },
        { "srgb_displayp3_to_lin_rec709", "sRGB Display P3 to Linear Rec. 709" },
        { "srgb_texture_to_lin_rec709", "sRGB Texture to Linear Rec. 709" },
        { "standard_surface_to_UsdPreviewSurface", "Standard Surface to USD Preview Surface" },
        { "standard_surface_to_gltf_pbr", "Standard Surface to glTF PBR" },
        { "standard_surface_to_open_pbr_surface", "Standard Surface to OpenPBR Surface" },
        { "subsurface_bsdf", "Subsurface BSDF" },
        { "surfacematerial", "Surface Material" },
        { "texcoord", "Texture Coordinate" },
        { "thin_film_bsdf", "Thin Film BSDF" },
        { "tiledcircles", "Tiled Circles" },
        { "tiledcloverleafs", "Tiled Cloverleafs" },
        { "tiledhexagons", "Tiled Hexagons" },
        { "tiledimage", "Tiled Image" },
        { "transformmatrix", "Transform Matrix" },
        { "transformnormal", "Transform Normal" },
        { "transformpoint", "Transform Point" },
        { "transformvector", "Transform Vector" },
        { "translucent_bsdf", "Translucent BSDF" },
        { "trianglewave", "Triangle Wave" },
        { "triplanarprojection", "Tri-planar Projection" },
        { "unifiednoise2d", "Unified 2D Noise" },
        { "unifiednoise3d", "Unified 3D Noise" },
        { "uniform_edf", "Uniform EDF" },
        { "unpremult", "Unpremultiply" },
        { "viewdirection", "View Direction" },
        { "volumematerial", "Volume Material" },
        { "worleynoise2d", "2D Worley (Voronoi) Noise" },
        { "worleynoise3d", "3D Worley (Voronoi) Noise" },
        // Those are category names associated with MaterialX:
        { "bxdf", "BXDF" },
        { "cmlib", "Color Transform" },
        { "colortransform", "Color Transform" },
        { "convolution2d", "Convolution 2D" },
        { "nprlib", "NPR" },
        { "pbr", "PBR" },
        { "pbrlib", "PBR" },
        { "procedural2d", "Procedural 2D" },
        { "procedural3d", "Procedural 3D" },
        { "stdlib", "Standard" },
        { "texture2d", "Texture 2D" },
        // These ones are from MayaUSD and also require manual expansion:
        { "LdkColorCorrect", "LookdevKit Color Correct" },
        { "LdkFloatCorrect", "LookdevKit Float Correct" },
        { "texcoordtangents", "Tangents from Texture Coordinates" },
        { "arbitrarytangents", "Arbitrary Tangents" },
        { "sRGBtoLinrec709", "sRGB to Linear Rec. 709" },
        { "sRGBtoACEScg", "sRGB to ACEScg" },
        { "sRGBtoACES2065", "sRGB to ACES 2065-1" },
        { "sRGBtoLinDCIP3D65", "sRGB to Linear DCI-P3 D65" },
        { "sRGBtoLinrec2020", "sRGB to Linear Rec. 2020" },
    };

    const auto it = kUINames.find(nodename);
    if (it != kUINames.end()) {
        return it->second;
    }
    return {};
}

} // namespace

std::string sanitizeName(const std::string& name)
{
    return PXR_NS::TfStringReplace(name, ":", "_");
}

std::string prettifyName(const std::string& name)
{
    // First try our temporarily hardcoded list:
    std::string prettyName = _getMaterialXUIName(name);

    if (!prettyName.empty()) {
        return prettyName;
    }

    // Note: slightly over-reserve to account for additional spaces.
    prettyName.reserve(name.size() + 6);
    size_t nbChars = name.size();
    bool   capitalizeNext = true;
    for (size_t i = 0; i < nbChars; ++i) {
        unsigned char nextLetter = name[i];
        if (std::isupper(name[i]) && i > 0 && !std::isdigit(name[i - 1])) {
            if ((i > 0 && (i < (nbChars - 1))
                 && (!std::isupper(name[i + 1]) && !std::isdigit(name[i + 1])))
                || std::islower(name[i - 1])) {
                if (prettyName.size() > 0 && prettyName.back() != ' ') {
                    prettyName += ' ';
                }
            }
            prettyName += nextLetter;
            capitalizeNext = false;
        } else if (name[i] == '_' || name[i] == ':') {
            if (prettyName.size() > 0 && prettyName.back() != ' ') {
                prettyName += " ";
            }
            capitalizeNext = true;
        } else {
            if (capitalizeNext) {
                nextLetter = std::toupper(nextLetter);
                capitalizeNext = false;
            }
            prettyName += nextLetter;
        }
    }
    // Manual substitutions for custom capitalisations. Will be searched as case-insensitive.
    static const std::unordered_map<std::string, std::string> subs = {
        { "usd", "USD" },
        { "mtlx", "MaterialX" },
        { "lookdevx", "LookdevX" },
    };

    static const auto subRegexes = []() {
        std::vector<std::pair<std::regex, std::string>> regexes;
        regexes.reserve(subs.size());
        for (auto&& kv : subs) {
            regexes.emplace_back(
                std::regex { "\\b" + kv.first + "\\b", std::regex_constants::icase }, kv.second);
        }
        return regexes;
    }();

    for (auto&& re : subRegexes) {
        prettyName = regex_replace(prettyName, re.first, re.second);
    }

    return prettyName;
}

std::vector<std::string> splitString(const std::string& str, const std::string& separators)
{
    std::vector<std::string> split;

    std::string::size_type lastPos = str.find_first_not_of(separators, 0);
    std::string::size_type pos = str.find_first_of(separators, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos) {
        split.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(separators, pos);
        pos = str.find_first_of(separators, lastPos);
    }

    return split;
}

} // namespace USDUFE_NS_DEF
