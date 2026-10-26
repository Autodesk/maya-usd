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
#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/gaussianSplatPly.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/colorSpaceAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdVol/particleField3DGaussianSplat.h>

#include <maya/MDagModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MNodeClass.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h>

#ifdef UFE_HAS_COLOR_MANAGEMENT_HANDLER
#include <mayaUsd/ufe/Global.h>

#include <ufe/colorManagementHandler.h>
#endif

#include <ghc/fs_std.hpp>

#include <algorithm>
#include <string>
#include <system_error>
#include <vector>

namespace MAYAUSD_NS_DEF {

using namespace PXR_NS;

namespace {

// Names of the mtoa aiGaussianSplat node and attributes.
namespace GaussianSplatMayaNames {
constexpr const char* kNodeType = "aiGaussianSplat";
constexpr const char* kFilenameAttr = "filename";
constexpr const char* kInputColorSpaceAttr = "inputColorSpace";
} // namespace GaussianSplatMayaNames

const char* const _kShapeNameSuffix = "Shape";

bool _IsGaussianSplatNodeTypeAvailable()
{
    MNodeClass nodeClass(GaussianSplatMayaNames::kNodeType);
    return nodeClass.typeId() != MTypeId();
}

// Reads per-particle data the way the UsdVol schema specifies: an array longer than expected is
// truncated, and a missing or shorter one is ignored, in which case the caller uses the default.
template <typename T>
bool _FitToParticleCount(const UsdAttribute& attr, size_t expectedSize, T* array)
{
    if (array->empty()) {
        return false;
    }
    if (array->size() < expectedSize) {
        TF_WARN(
            "%s: has %zu elements but %zu are expected; using the default value instead.",
            attr.GetPath().GetText(),
            array->size(),
            expectedSize);
        return false;
    }
    array->resize(expectedSize);
    return true;
}

void _Fill(const VtVec3fArray& array, std::vector<float>* out)
{
    out->clear();
    out->reserve(3 * array.size());
    for (const GfVec3f& v : array) {
        out->insert(out->end(), v.data(), v.data() + 3);
    }
}

// GfQuat stores (imaginary, real); SplatCloud stores (w, x, y, z).
void _Fill(const VtQuatfArray& array, std::vector<float>* out)
{
    out->clear();
    out->reserve(4 * array.size());
    for (const GfQuatf& q : array) {
        const GfVec3f& im = q.GetImaginary();
        out->push_back(q.GetReal());
        out->insert(out->end(), im.data(), im.data() + 3);
    }
}

// The positions define the particle count. The other attributes are optional and fall back to
// the schema defaults when they are missing or too short.
bool _FillSplatCloud(
    const UsdVolParticleField3DGaussianSplat& splat,
    MayaUsd::utils::SplatCloud*               cloud,
    UsdAttribute*                             radianceAttr)
{
    // The UsesFloat* methods pick the float or half attribute, whichever is authored. Values
    // are read as VtValue and cast, so the half arrays are converted to float.
    const UsdTimeCode time = UsdTimeCode::EarliestTime();
    UsdAttribute      attr;
    VtValue           value;

    splat.UsesFloatPositions(&attr);
    attr.Get(&value, time);
    const VtVec3fArray positions
        = VtValue::Cast<VtVec3fArray>(value).GetWithDefault<VtVec3fArray>();
    if (positions.empty()) {
        TF_WARN("%s: could not read the Gaussian splat positions.", splat.GetPath().GetText());
        return false;
    }
    const size_t count = positions.size();
    _Fill(positions, &cloud->_positions);

    splat.UsesFloatScales(&attr);
    attr.Get(&value, time);
    VtVec3fArray scales = VtValue::Cast<VtVec3fArray>(value).GetWithDefault<VtVec3fArray>();
    if (!_FitToParticleCount(attr, count, &scales)) {
        scales.assign(count, GfVec3f(1.0f));
    }
    _Fill(scales, &cloud->_scales);

    // VtValue has no half to float cast for quaternions, so convert them here.
    VtQuatfArray orientations;
    if (splat.UsesFloatOrientations(&attr)) {
        attr.Get(&orientations, time);
    } else {
        VtQuathArray halfOrientations;
        attr.Get(&halfOrientations, time);
        orientations.assign(halfOrientations.cbegin(), halfOrientations.cend());
    }
    if (!_FitToParticleCount(attr, count, &orientations)) {
        orientations.assign(count, GfQuatf::GetIdentity());
    }
    _Fill(orientations, &cloud->_rotations);

    splat.UsesFloatOpacities(&attr);
    attr.Get(&value, time);
    VtFloatArray opacities = VtValue::Cast<VtFloatArray>(value).GetWithDefault<VtFloatArray>();
    if (!_FitToParticleCount(attr, count, &opacities)) {
        opacities.assign(count, 1.0f);
    }
    cloud->_opacities.assign(opacities.cbegin(), opacities.cend());

    // The schema fallback for the degree is 3.
    int shDegree = 3;
    splat.GetRadianceSphericalHarmonicsDegreeAttr().Get(&shDegree, time);
    splat.UsesFloatRadianceCoefficients(radianceAttr);
    radianceAttr->Get(&value, time);
    VtVec3fArray coeffs = VtValue::Cast<VtVec3fArray>(value).GetWithDefault<VtVec3fArray>();
    if (shDegree < 0 || shDegree > 3) {
        TF_WARN(
            "%s: spherical harmonics degree %d is not supported; using the default color.",
            splat.GetPath().GetText(),
            shDegree);
        coeffs.clear();
    } else if (!_FitToParticleCount(
                   *radianceAttr, count * MayaUsd::utils::splatShCoeffCount(shDegree), &coeffs)) {
        coeffs.clear();
    }
    if (coeffs.empty() && count > 0) {
        // The schema default is a DC signal of (0.5, 0.5, 0.5). The color is
        // 0.5 + C0 * coefficient, so that is a zero coefficient at degree 0.
        shDegree = 0;
        coeffs.assign(count, GfVec3f(0.0f));
        *radianceAttr = UsdAttribute();
    }
    cloud->_shDegree = shDegree;
    _Fill(coeffs, &cloud->_shCoeffs);
    return true;
}

// Maps a USD color space name to the name of a color space known to Maya, or returns an empty
// string if there is none.
std::string _GetMayaColorSpace(const TfToken& usdColorSpace)
{
    const std::string& name = usdColorSpace.GetString();

#ifdef UFE_HAS_COLOR_MANAGEMENT_HANDLER
    // The handler recognizes OCIO names and aliases, including the standard USD names.
    if (const auto cmHandler
        = Ufe::ColorManagementHandler::colorManagementHandler(MayaUsd::ufe::getMayaRunTimeId())) {
        if (cmHandler->isKnownColorSpace(name)) {
            return cmHandler->getCanonicalName(name);
        }
    }
#endif

    MStringArray mayaSpaces;
    MGlobal::executeCommand("colorManagementPrefs -q -inputSpaceNames", mayaSpaces);
    const bool isKnownToMaya
        = std::any_of(mayaSpaces.begin(), mayaSpaces.end(), [&](const MString& mayaSpace) {
              return name == mayaSpace.asChar();
          });
    return isKnownToMaya ? name : std::string();
}

// The .ply goes in the Maya workspace "data" file rule folder:
//     <workspace>/<data rule>/<usdStem>_<primPath>[_<n>].ply
// A numeric suffix is added to get a unique name.
std::string _GetSplatPlyPath(const UsdPrim& prim)
{
    const std::string workspace = UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar();

    std::string dataRule
        = MGlobal::executeCommandStringResult("workspace -q -fileRuleEntry \"data\"").asChar();
    if (dataRule.empty()) {
        dataRule = "data";
    }

    fs::filesystem::path dataDir(dataRule);
    if (!dataDir.is_absolute()) {
        dataDir = fs::filesystem::path(workspace) / dataDir;
    }

    std::error_code ec;
    fs::filesystem::create_directories(dataDir, ec);
    if (ec) {
        TF_RUNTIME_ERROR(
            "Could not create directory '%s': %s",
            dataDir.generic_string().c_str(),
            ec.message().c_str());
        return {};
    }

    std::string stem;
    if (const SdfLayerHandle rootLayer = prim.GetStage()->GetRootLayer()) {
        stem = fs::filesystem::path(rootLayer->GetRealPath()).stem().generic_string();
    }
    if (stem.empty()) {
        stem = "anon";
    }

    const std::string baseName = stem + TfStringReplace(prim.GetPath().GetString(), "/", "_");

    fs::filesystem::path plyPath = dataDir / (baseName + ".ply");
    for (int suffix = 1; fs::filesystem::exists(plyPath, ec); ++suffix) {
        plyPath = dataDir / (baseName + "_" + std::to_string(suffix) + ".ply");
    }

    return plyPath.generic_string();
}

} // namespace

PXRUSDMAYA_DEFINE_READER(UsdVolParticleField3DGaussianSplat, args, context)
{
    const UsdPrim& prim = args.GetUsdPrim();
    const SdfPath  primPath = prim.GetPath();

    if (!_IsGaussianSplatNodeTypeAvailable()) {
        TF_WARN(
            "%s: the %s node type is not available (is mtoa loaded?); skipping.",
            primPath.GetText(),
            GaussianSplatMayaNames::kNodeType);
        return false;
    }

    const UsdVolParticleField3DGaussianSplat splat(prim);

    MayaUsd::utils::SplatCloud cloud;
    UsdAttribute               radianceAttr;
    if (!_FillSplatCloud(splat, &cloud, &radianceAttr)) {
        TF_WARN("%s: could not read the Gaussian splat data; skipping.", primPath.GetText());
        return false;
    }
    if (cloud.count() == 0 || !cloud.isValid()) {
        TF_WARN("%s: contains no valid splat data; skipping.", primPath.GetText());
        return false;
    }

    MStatus status;
    MObject parentNode = context.GetMayaNode(primPath.GetParentPath(), true);
    MObject transformObj;
    if (!UsdMayaTranslatorUtil::CreateTransformNode(
            prim, parentNode, args, &context, &status, &transformObj)) {
        return false;
    }

    MDagModifier& dagMod = MayaUsd::MDagModifierUndoItem::create("Gaussian splat creation");
    MObject shapeObj = dagMod.createNode(GaussianSplatMayaNames::kNodeType, transformObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);
    TF_VERIFY(!shapeObj.isNull());

    MFnDependencyNode depFn(shapeObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    const std::string shapeName = prim.GetName().GetString() + _kShapeNameSuffix;
    depFn.setName(shapeName.c_str(), false, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    context.RegisterNewMayaNode(primPath.AppendChild(TfToken(shapeName)).GetString(), shapeObj);

    const std::string plyPath = _GetSplatPlyPath(prim);
    if (plyPath.empty()) {
        return false;
    }

    std::string errorMsg;
    if (!MayaUsd::utils::saveSplatPly(cloud, plyPath, &errorMsg)) {
        TF_RUNTIME_ERROR("%s: %s", primPath.GetText(), errorMsg.c_str());
        return false;
    }

    // Store the absolute path. Arnold does not resolve project-relative paths.
    MPlug filenamePlug = depFn.findPlug(GaussianSplatMayaNames::kFilenameAttr, true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = filenamePlug.setString(plyPath.c_str());
    CHECK_MSTATUS_AND_RETURN(status, false);

    // The node has its own input color space, which we set from the color space of the
    // radiance attribute.
    const TfToken usdColorSpace
        = radianceAttr ? UsdColorSpaceAPI::ComputeColorSpaceName(radianceAttr) : TfToken();
    if (!usdColorSpace.IsEmpty()) {
        const std::string mayaColorSpace = _GetMayaColorSpace(usdColorSpace);
        if (mayaColorSpace.empty()) {
            TF_WARN(
                "%s: color space '%s' is not recognized by Maya; using the node default.",
                primPath.GetText(),
                usdColorSpace.GetText());
        } else {
            MPlug colorSpacePlug
                = depFn.findPlug(GaussianSplatMayaNames::kInputColorSpaceAttr, true, &status);
            if (status == MS::kSuccess && !colorSpacePlug.isNull()) {
                status = colorSpacePlug.setString(mayaColorSpace.c_str());
            }
            if (status != MS::kSuccess) {
                TF_WARN(
                    "%s: could not set %s to '%s': %s",
                    primPath.GetText(),
                    GaussianSplatMayaNames::kInputColorSpaceAttr,
                    mayaColorSpace.c_str(),
                    status.errorString().asChar());
            }
        }
    }

    return true;
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
