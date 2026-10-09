//
// Copyright 2026 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "gaussianSplatPly.h"

#define TINYPLY_IMPLEMENTATION
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <tinyply.h>

namespace MAYAUSD_NS_DEF {
namespace utils {

namespace {

template <typename T> void convertToFloat(const uint8_t* src, float* dst, size_t n)
{
    std::vector<T> values(n);
    std::copy_n(src, n * sizeof(T), reinterpret_cast<uint8_t*>(values.data()));
    std::transform(values.begin(), values.end(), dst, [](T v) { return static_cast<float>(v); });
}

bool toFloats(
    const std::shared_ptr<tinyply::PlyData>& data,
    size_t                                   nProps,
    const char*                              label,
    std::vector<float>*                      out,
    std::string*                             errorMsg)
{
    if (!data) {
        *errorMsg = std::string("missing ") + label;
        return false;
    }
    if (data->isList) {
        *errorMsg = std::string(label) + " is a list property";
        return false;
    }

    auto info = tinyply::PropertyTable.find(data->t);
    if (info == tinyply::PropertyTable.end() || data->t == tinyply::Type::INVALID) {
        *errorMsg = std::string(label) + " has an invalid type";
        return false;
    }

    const size_t n = data->count * nProps;
    const size_t stride = static_cast<size_t>(info->second.stride);
    if (data->buffer.size_bytes() != n * stride) {
        *errorMsg = std::string("size mismatch for ") + label;
        return false;
    }

    out->resize(n);
    const uint8_t* src = data->buffer.get_const();
    float*         dst = out->data();

    switch (data->t) {
    case tinyply::Type::FLOAT32:
        std::copy_n(src, n * sizeof(float), reinterpret_cast<uint8_t*>(dst));
        break;
    case tinyply::Type::FLOAT64: convertToFloat<double>(src, dst, n); break;
    case tinyply::Type::INT8: convertToFloat<int8_t>(src, dst, n); break;
    case tinyply::Type::UINT8: convertToFloat<uint8_t>(src, dst, n); break;
    case tinyply::Type::INT16: convertToFloat<int16_t>(src, dst, n); break;
    case tinyply::Type::UINT16: convertToFloat<uint16_t>(src, dst, n); break;
    case tinyply::Type::INT32: convertToFloat<int32_t>(src, dst, n); break;
    case tinyply::Type::UINT32: convertToFloat<uint32_t>(src, dst, n); break;
    default: *errorMsg = std::string(label) + " has an unsupported type"; return false;
    }
    return true;
}

} // namespace

bool SplatCloud::isValid() const
{
    if (_shDegree < 0 || _shDegree > 3)
        return false;
    const size_t n = count();
    const size_t coeffCount = static_cast<size_t>(shCoeffCount());
    return _positions.size() == n * 3 && _scales.size() == n * 3 && _rotations.size() == n * 4
        && _shCoeffs.size() == n * coeffCount * 3;
}

bool loadSplatPly(const std::string& plyPath, SplatCloud* out, std::string* errorMsg)
{
    std::string localError {};
    if (!errorMsg)
        errorMsg = &localError;

    if (!out) {
        *errorMsg = plyPath + ": null output cloud";
        return false;
    }

    std::shared_ptr<tinyply::PlyData> positions, scales, rotations, opacities, dc, rest;
    size_t                            numRest = 0;
    int                               shDegree = 0;

    std::vector<float> posF, scaleF, rotF, opF, dcF, restF;
    try {
        std::ifstream stream(plyPath, std::ios::binary);
        if (!stream) {
            *errorMsg = plyPath + ": could not open file";
            return false;
        }

        tinyply::PlyFile file;
        if (!file.parse_header(stream)) {
            *errorMsg = plyPath + ": malformed PLY header";
            return false;
        }

        const auto& elements = file.get_elements();
        const auto  vertexIt = std::find_if(
            elements.begin(), elements.end(), [](const auto& e) { return e.name == "vertex"; });

        if (vertexIt == elements.end()) {
            *errorMsg = plyPath + ": no vertex element";
            return false;
        }
        // Determine SH degree from f_rest_* count
        numRest = std::count_if(
            vertexIt->properties.begin(), vertexIt->properties.end(), [](const auto& p) {
                return p.name.rfind("f_rest_", 0) == 0;
            });

        if (numRest % 3 != 0) {
            *errorMsg = plyPath + ": " + "f_rest count not divisible by 3";
            return false;
        }

        switch (numRest / 3) {
        case 0: shDegree = 0; break;
        case 3: shDegree = 1; break;
        case 8: shDegree = 2; break;
        case 15: shDegree = 3; break;
        default: {
            *errorMsg = plyPath + ": " + "unsupported SH coefficient count";
            return false;
        }
        }

        // request_properties_from_element throws if a property is missing.
        positions = file.request_properties_from_element("vertex", { "x", "y", "z" });
        scales
            = file.request_properties_from_element("vertex", { "scale_0", "scale_1", "scale_2" });
        rotations = file.request_properties_from_element(
            "vertex", { "rot_0", "rot_1", "rot_2", "rot_3" });
        opacities = file.request_properties_from_element("vertex", { "opacity" });
        dc = file.request_properties_from_element("vertex", { "f_dc_0", "f_dc_1", "f_dc_2" });

        if (numRest > 0) {
            std::vector<std::string> restNames;
            restNames.reserve(numRest);
            for (size_t i = 0; i < numRest; ++i)
                restNames.push_back("f_rest_" + std::to_string(i));
            rest = file.request_properties_from_element("vertex", restNames);
        }

        file.read(stream);

        if (!toFloats(positions, 3, "position", &posF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }
        if (!toFloats(scales, 3, "scale", &scaleF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }
        if (!toFloats(rotations, 4, "rotation", &rotF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }
        if (!toFloats(opacities, 1, "opacity", &opF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }
        if (!toFloats(dc, 3, "f_dc", &dcF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }
        if (rest && !toFloats(rest, numRest, "f_rest", &restF, errorMsg)) {
            *errorMsg = plyPath + ": " + *errorMsg;
            return false;
        }

    } catch (const std::exception& e) {
        *errorMsg = plyPath + ": " + e.what();
        return false;
    }
    const size_t vertexCount = positions->count;
    const size_t coeffCount = static_cast<size_t>((shDegree + 1) * (shDegree + 1)); // includes DC
    const size_t k = coeffCount - 1; // number of non-DC coefficients per channel

    SplatCloud result;
    result._shDegree = shDegree;
    result._positions = std::move(posF);
    result._scales = std::move(scaleF);
    result._rotations = std::move(rotF);
    result._opacities = std::move(opF);
    result._shCoeffs.assign(vertexCount * coeffCount * 3, 0.0f);

    for (size_t i = 0; i < vertexCount; ++i) {
        result._scales[i * 3 + 0] = std::exp(result._scales[i * 3 + 0]);
        result._scales[i * 3 + 1] = std::exp(result._scales[i * 3 + 1]);
        result._scales[i * 3 + 2] = std::exp(result._scales[i * 3 + 2]);

        // Rotation normalization
        float w = result._rotations[i * 4 + 0];
        float x = result._rotations[i * 4 + 1];
        float y = result._rotations[i * 4 + 2];
        float z = result._rotations[i * 4 + 3];
        float lengthSq = w * w + x * x + y * y + z * z;
        if (lengthSq < std::numeric_limits<float>::epsilon()) {
            result._rotations[i * 4 + 0] = 1.0f;
            result._rotations[i * 4 + 1] = 0.0f;
            result._rotations[i * 4 + 2] = 0.0f;
            result._rotations[i * 4 + 3] = 0.0f;
        } else {
            float invLength = 1.0f / std::sqrt(lengthSq);
            result._rotations[i * 4 + 0] = w * invLength;
            result._rotations[i * 4 + 1] = x * invLength;
            result._rotations[i * 4 + 2] = y * invLength;
            result._rotations[i * 4 + 3] = z * invLength;
        }

        // Sigmoid activation: sigmoid(x) = 1 / (1 + exp(-x))
        // Avoid overflow in exp() for large-magnitude logits.
        float opacityRaw = result._opacities[i];
        if (opacityRaw >= 0.0f) {
            result._opacities[i] = 1.0f / (1.0f + std::exp(-opacityRaw));
        } else {
            float e = std::exp(opacityRaw);
            result._opacities[i] = e / (1.0f + e);
        }

        // The DC is band is the first coefficeient before other SH
        result._shCoeffs[i * coeffCount * 3 + 0] = dcF[i * 3 + 0];
        result._shCoeffs[i * coeffCount * 3 + 1] = dcF[i * 3 + 1];
        result._shCoeffs[i * coeffCount * 3 + 2] = dcF[i * 3 + 2];

        // f_rest is channel-major in ply (all R, then all G, then all B);
        // Reorder to coefficient-major, [gaussian][coeff][channel].
        for (size_t c = 0; c < k; ++c) {
            float* dst = &result._shCoeffs[i * coeffCount * 3 + (c + 1) * 3];
            dst[0] = restF[i * (3 * k) + c];
            dst[1] = restF[i * (3 * k) + k + c];
            dst[2] = restF[i * (3 * k) + 2 * k + c];
        }
    }

    if (!result.isValid()) {
        *errorMsg = plyPath + ": internal error building SplatCloud";
        return false;
    }

    *out = std::move(result);
    return true;
}

bool saveSplatPly(
    const SplatCloud&  cloud,
    const std::string& path,
    std::string*       errorMsg,
    bool               binary)
{
    std::string localError;
    if (!errorMsg)
        errorMsg = &localError;

    if (!cloud.isValid()) {
        *errorMsg = "SplatCloud arrays are inconsistent with gaussian count and SH degree";
        return false;
    }

    const size_t n = cloud.count();
    const size_t coeffCount = static_cast<size_t>(cloud.shCoeffCount());
    const size_t M = coeffCount - 1; // higher-order coefficients, excluding DC
    const size_t numRest = M * 3;

    // tinyply stores pointers to these arrays, they must stay alive until
    // file.write() below has finished.
    std::vector<float> normals(n * 3, 0.0f);
    std::vector<float> dc(n * 3);
    std::vector<float> rest(n * numRest);
    std::vector<float> opac(n);
    std::vector<float> scale(n * 3);

    // Undo activations
    const float eps = std::numeric_limits<float>::epsilon();
    for (size_t i = 0; i < n; ++i) {
        // exp() -> log(); guard against zero/negative scales.
        scale[i * 3 + 0]
            = std::log(std::max(cloud._scales[i * 3 + 0], std::numeric_limits<float>::min()));
        scale[i * 3 + 1]
            = std::log(std::max(cloud._scales[i * 3 + 1], std::numeric_limits<float>::min()));
        scale[i * 3 + 2]
            = std::log(std::max(cloud._scales[i * 3 + 2], std::numeric_limits<float>::min()));

        // Reorder coefficient
        // DC band is coefficient 0, stored raw -- copy straight through.
        dc[i * 3 + 0] = cloud._shCoeffs[i * coeffCount * 3 + 0];
        dc[i * 3 + 1] = cloud._shCoeffs[i * coeffCount * 3 + 1];
        dc[i * 3 + 2] = cloud._shCoeffs[i * coeffCount * 3 + 2];

        // sigmoid() -> logit(); clamped
        const float opacity = std::min(std::max(cloud._opacities[i], eps), 1.0f - eps);
        opac[i] = std::log(opacity / (1.0f - opacity));

        // Reorder SH to channel major
        for (size_t c = 0; c < M; ++c) {
            const float* src = &cloud._shCoeffs[i * coeffCount * 3 + (c + 1) * 3];
            rest[i * numRest + 0 * M + c] = src[0];
            rest[i * numRest + 1 * M + c] = src[1];
            rest[i * numRest + 2 * M + c] = src[2];
        }
    }

    const std::vector<float>& pos = cloud._positions;
    const std::vector<float>& rot = cloud._rotations;

    std::filebuf fb;
    fb.open(path, binary ? (std::ios::out | std::ios::binary) : std::ios::out);
    std::ostream outstream(&fb);
    if (!fb.is_open() || outstream.fail()) {
        *errorMsg = path + ": failed to open for writing";
        return false;
    }
    if (!binary) {
        outstream.precision(std::numeric_limits<float>::max_digits10);
    }
    try {
        tinyply::PlyFile file {};
        file.add_properties_to_element(
            "vertex",
            { "x", "y", "z" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(const_cast<float*>(pos.data())),
            tinyply::Type::INVALID,
            0);

        file.add_properties_to_element(
            "vertex",
            { "nx", "ny", "nz" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(normals.data()),
            tinyply::Type::INVALID,
            0);

        file.add_properties_to_element(
            "vertex",
            { "f_dc_0", "f_dc_1", "f_dc_2" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(dc.data()),
            tinyply::Type::INVALID,
            0);

        if (numRest > 0) {
            std::vector<std::string> names;
            names.reserve(numRest);
            for (size_t i = 0; i < numRest; ++i) {
                names.push_back("f_rest_" + std::to_string(i));
            }
            file.add_properties_to_element(
                "vertex",
                names,
                tinyply::Type::FLOAT32,
                n,
                reinterpret_cast<uint8_t*>(rest.data()),
                tinyply::Type::INVALID,
                0);
        }

        file.add_properties_to_element(
            "vertex",
            { "opacity" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(opac.data()),
            tinyply::Type::INVALID,
            0);

        file.add_properties_to_element(
            "vertex",
            { "scale_0", "scale_1", "scale_2" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(scale.data()),
            tinyply::Type::INVALID,
            0);

        file.add_properties_to_element(
            "vertex",
            { "rot_0", "rot_1", "rot_2", "rot_3" },
            tinyply::Type::FLOAT32,
            n,
            reinterpret_cast<uint8_t*>(const_cast<float*>(rot.data())),
            tinyply::Type::INVALID,
            0);

        file.write(outstream, binary);
    } catch (const std::exception& e) {
        *errorMsg = path + ": " + e.what();
        return false;
    }

    outstream.flush();
    if (outstream.fail() || !fb.close()) {
        *errorMsg = path + ": failed to write";
        return false;
    }
    return true;
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
