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
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/gaussianSplatPly.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
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

namespace {

// Names of the mtoa aiGaussianSplat node and attributes.
namespace GaussianSplatMayaNames {
constexpr const char* kNodeType = "aiGaussianSplat";
constexpr const char* kFilenameAttr = "filename";
constexpr const char* kInputColorSpaceAttr = "inputColorSpace";
} // namespace GaussianSplatMayaNames

const char* const _kShapeNameSuffix = "Shape";

bool isArnoldGaussianSplatNodeAvailable()
{
    MNodeClass nodeClass(GaussianSplatMayaNames::kNodeType);
    return nodeClass.typeId() != MTypeId();
}

// Reads per-particle data the way the UsdVol schema specifies: an array longer than expected is
// truncated, and a missing or shorter one is ignored, in which case the caller uses the default.
template <typename T>
bool fitToParticleCount(const PXR_NS::UsdAttribute& attr, size_t expectedSize, T* array)
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

// Reads an attribute into a fresh array. The result is empty if the attribute is invalid, has no
// value, or its value cannot be converted to the array type.
template <typename T> T readArray(const PXR_NS::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    PXR_NS::VtValue value;
    if (!attr || !attr.Get(&value, time)) {
        return T();
    }
    return PXR_NS::VtValue::Cast<T>(value).template GetWithDefault<T>();
}

void fill(const PXR_NS::VtVec3fArray& array, std::vector<float>* out)
{
    static_assert(
        sizeof(PXR_NS::GfVec3f) == 3 * sizeof(float), "PXR_NS::GfVec3f must be tightly packed");
    const float* data = array.empty() ? nullptr : array.cdata()->data();
    out->assign(data, data + 3 * array.size());
}

// GfQuat stores (imaginary, real); SplatCloud stores (w, x, y, z).
void fill(const PXR_NS::VtQuatfArray& array, std::vector<float>* out)
{
    out->clear();
    out->reserve(4 * array.size());
    for (const PXR_NS::GfQuatf& q : array) {
        const PXR_NS::GfVec3f& im = q.GetImaginary();
        out->push_back(q.GetReal());
        out->insert(out->end(), im.data(), im.data() + 3);
    }
}

// The positions define the particle count. The other attributes are optional and fall back to
// the schema defaults when they are missing or too short.
bool fillSplatCloud(
    const PXR_NS::UsdVolParticleField3DGaussianSplat& splat,
    MayaUsd::utils::SplatCloud*                       cloud,
    PXR_NS::UsdAttribute*                             radianceAttr)
{
    // The UsesFloat* methods pick the float or half attribute, whichever is authored. Values
    // are read as PXR_NS::VtValue and cast, so the half arrays are converted to float.
    const PXR_NS::UsdTimeCode time = PXR_NS::UsdTimeCode::Default();
    PXR_NS::UsdAttribute      attr;

    splat.UsesFloatPositions(&attr);
    const PXR_NS::VtVec3fArray positions = readArray<PXR_NS::VtVec3fArray>(attr, time);
    if (positions.empty()) {
        TF_WARN("%s: could not read the Gaussian splat positions.", splat.GetPath().GetText());
        return false;
    }
    const size_t count = positions.size();
    fill(positions, &cloud->_positions);

    splat.UsesFloatScales(&attr);
    PXR_NS::VtVec3fArray scales = readArray<PXR_NS::VtVec3fArray>(attr, time);
    if (!fitToParticleCount(attr, count, &scales)) {
        scales.assign(count, PXR_NS::GfVec3f(1.0f));
    }
    fill(scales, &cloud->_scales);

    // PXR_NS::VtValue has no half to float cast for quaternions, so convert them here.
    PXR_NS::VtQuatfArray orientations;
    if (splat.UsesFloatOrientations(&attr)) {
        orientations = readArray<PXR_NS::VtQuatfArray>(attr, time);
    } else {
        const PXR_NS::VtQuathArray halfOrientations = readArray<PXR_NS::VtQuathArray>(attr, time);
        orientations.assign(halfOrientations.cbegin(), halfOrientations.cend());
    }
    if (!fitToParticleCount(attr, count, &orientations)) {
        orientations.assign(count, PXR_NS::GfQuatf::GetIdentity());
    }
    fill(orientations, &cloud->_rotations);

    splat.UsesFloatOpacities(&attr);
    PXR_NS::VtFloatArray opacities = readArray<PXR_NS::VtFloatArray>(attr, time);
    if (!fitToParticleCount(attr, count, &opacities)) {
        opacities.assign(count, 1.0f);
    }
    cloud->_opacities.assign(opacities.cbegin(), opacities.cend());

    // The schema fallback for the degree is 3.
    int shDegree = 3;
    splat.GetRadianceSphericalHarmonicsDegreeAttr().Get(&shDegree, time);
    splat.UsesFloatRadianceCoefficients(radianceAttr);
    PXR_NS::VtVec3fArray coeffs = readArray<PXR_NS::VtVec3fArray>(*radianceAttr, time);
    if (shDegree < 0 || shDegree > 3) {
        TF_WARN(
            "%s: spherical harmonics degree %d is not supported; using the default color.",
            splat.GetPath().GetText(),
            shDegree);
        coeffs.clear();
    } else if (!fitToParticleCount(
                   *radianceAttr, count * MayaUsd::utils::splatShCoeffCount(shDegree), &coeffs)) {
        coeffs.clear();
    }
    if (coeffs.empty() && count > 0) {
        // The schema default is a DC signal of (0.5, 0.5, 0.5). The color is
        // 0.5 + C0 * coefficient, so that is a zero coefficient at degree 0.
        shDegree = 0;
        coeffs.assign(count, PXR_NS::GfVec3f(0.0f));
        *radianceAttr = PXR_NS::UsdAttribute();
    }
    cloud->_shDegree = shDegree;
    fill(coeffs, &cloud->_shCoeffs);
    return true;
}

// Maps a USD color space name to the name of a color space known to Maya, or returns an empty
// string if there is none.
std::string getMayaColorSpace(const PXR_NS::TfToken& usdColorSpace)
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

// The .ply goes in the Maya workspace "diskCache" file rule folder, falling back to "data":
//     <workspace>/<diskCache rule>/<usdStem>_<primPath>.ply
// A suffix is added to get a unique name. This only builds the path: it creates nothing on disk.
std::string getSplatPlyPath(const PXR_NS::UsdPrim& prim, fs::filesystem::path* dataDir)
{
    const std::string workspace = PXR_NS::UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar();

    std::string dataRule
        = MGlobal::executeCommandStringResult("workspace -q -fileRuleEntry \"diskCache\"").asChar();
    if (dataRule.empty()) {
        dataRule = "data";
    }

    *dataDir = fs::filesystem::path(dataRule);
    if (!dataDir->is_absolute()) {
        *dataDir = fs::filesystem::path(workspace) / *dataDir;
    }

    std::string stem;
    if (const PXR_NS::SdfLayerHandle rootLayer = prim.GetStage()->GetRootLayer()) {
        stem = fs::filesystem::path(rootLayer->GetRealPath()).stem().generic_string();
    }
    if (stem.empty()) {
        stem = "anon";
    }

    const std::string baseName
        = stem + PXR_NS::TfStringReplace(prim.GetPath().GetString(), "/", "_");
    return PXR_NS::UsdMayaUtilFileSystem::ensureUniqueFileName(
        (*dataDir / (baseName + ".ply")).generic_string());
}

bool writeSplatPly(
    const MayaUsd::utils::SplatCloud& cloud,
    const fs::filesystem::path&       dataDir,
    const std::string&                plyPath,
    std::string*                      errorMsg)
{
    std::error_code ec;
    fs::filesystem::create_directories(dataDir, ec);
    if (ec) {
        *errorMsg
            = "Could not create directory '" + dataDir.generic_string() + "': " + ec.message();
        return false;
    }

    if (!MayaUsd::utils::saveSplatPly(cloud, plyPath, errorMsg)) {
        fs::filesystem::remove(fs::filesystem::path(plyPath), ec);
        return false;
    }
    return true;
}

} // namespace

} // namespace MAYAUSD_NS_DEF

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_DEFINE_READER(UsdVolParticleField3DGaussianSplat, args, context)
{
    const UsdPrim& prim = args.GetUsdPrim();
    const SdfPath  primPath = prim.GetPath();

    if (!MayaUsd::isArnoldGaussianSplatNodeAvailable()) {
        TF_WARN(
            "%s: the %s node type is not available (is mtoa loaded?); skipping.",
            primPath.GetText(),
            MayaUsd::GaussianSplatMayaNames::kNodeType);
        return false;
    }

    const UsdVolParticleField3DGaussianSplat splat(prim);

    MayaUsd::utils::SplatCloud cloud;
    UsdAttribute               radianceAttr;
    if (!MayaUsd::fillSplatCloud(splat, &cloud, &radianceAttr)) {
        TF_WARN("%s: could not read the Gaussian splat data; skipping.", primPath.GetText());
        return false;
    }
    if (cloud.count() == 0 || !cloud.isValid()) {
        TF_WARN("%s: contains no valid splat data; skipping.", primPath.GetText());
        return false;
    }

    // Write the .ply first so that no node is left behind if it cannot be written. The Arnold node
    // can only load its data from a file, so the splats have to be saved to disk.
    fs::filesystem::path dataDir;
    const std::string    plyPath = MayaUsd::getSplatPlyPath(prim, &dataDir);
    std::string          errorMsg;
    if (!MayaUsd::writeSplatPly(cloud, dataDir, plyPath, &errorMsg)) {
        TF_RUNTIME_ERROR("%s: %s", primPath.GetText(), errorMsg.c_str());
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
    MObject       shapeObj
        = dagMod.createNode(MayaUsd::GaussianSplatMayaNames::kNodeType, transformObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);
    TF_VERIFY(!shapeObj.isNull());

    MFnDependencyNode depFn(shapeObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    const std::string shapeName = prim.GetName().GetString() + MayaUsd::_kShapeNameSuffix;
    depFn.setName(shapeName.c_str(), false, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    context.RegisterNewMayaNode(primPath.AppendChild(TfToken(shapeName)).GetString(), shapeObj);

    MPlug filenamePlug
        = depFn.findPlug(MayaUsd::GaussianSplatMayaNames::kFilenameAttr, true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = filenamePlug.setString(plyPath.c_str());
    CHECK_MSTATUS_AND_RETURN(status, false);

    // The node has its own input color space, which we set from the color space of the
    // radiance attribute.
    const TfToken usdColorSpace
        = radianceAttr ? UsdColorSpaceAPI::ComputeColorSpaceName(radianceAttr) : TfToken();
    if (!usdColorSpace.IsEmpty()) {
        const std::string mayaColorSpace = MayaUsd::getMayaColorSpace(usdColorSpace);
        if (mayaColorSpace.empty()) {
            TF_WARN(
                "%s: color space '%s' is not recognized by Maya; using the node default.",
                primPath.GetText(),
                usdColorSpace.GetText());
        } else {
            MPlug colorSpacePlug = depFn.findPlug(
                MayaUsd::GaussianSplatMayaNames::kInputColorSpaceAttr, true, &status);
            if (status == MS::kSuccess && !colorSpacePlug.isNull()) {
                status = colorSpacePlug.setString(mayaColorSpace.c_str());
            }
            if (status != MS::kSuccess) {
                TF_WARN(
                    "%s: could not set %s to '%s': %s",
                    primPath.GetText(),
                    MayaUsd::GaussianSplatMayaNames::kInputColorSpaceAttr,
                    mayaColorSpace.c_str(),
                    status.errorString().asChar());
            }
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
