//
// Copyright 2020 Autodesk
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
#include "shadingTokens.h"
#include "usdReflectWriter.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_PhongWriter : public PxrUsdTranslators_ReflectWriter
{
    typedef PxrUsdTranslators_ReflectWriter BaseClass;

public:
    PxrUsdTranslators_PhongWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(phong, PxrUsdTranslators_PhongWriter);

PxrUsdTranslators_PhongWriter::PxrUsdTranslators_PhongWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : PxrUsdTranslators_ReflectWriter(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
void PxrUsdTranslators_PhongWriter::Write(const UsdTimeCode& usdTime)
{
    BaseClass::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    MPlug cosinePowerPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaTokens->cosinePower.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess) {
        float cosinePower = cosinePowerPlug.asFloat();

        // In the maya UI, cosinePower goes from 2.0 to 100.0
        // this does not map directly to specular roughness
        float roughnessFloat = std::sqrt(1.0f / (0.454f * cosinePower + 3.357f));

        shaderSchema
            .CreateInput(
                PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName, SdfValueTypeNames->Float)
            .Set(roughnessFloat, usdTime);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
