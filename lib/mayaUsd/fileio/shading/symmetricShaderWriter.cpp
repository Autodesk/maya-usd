//
// Copyright 2020 Pixar
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
#include "symmetricShaderWriter.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


/* static */
void
UsdMayaSymmetricShaderWriter::RegisterWriter(
        const TfToken& mayaNodeTypeName,
        const TfToken& usdShaderId,
        const TfToken& materialConversionName)
{
    UsdMayaShaderWriterRegistry::Register(
        mayaNodeTypeName,
        [materialConversionName](const UsdMayaJobExportArgs& exportArgs) {
            return UsdMayaSymmetricShaderWriter::CanExport(
                exportArgs,
                materialConversionName);
        },
        [usdShaderId](
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx) {
            return std::make_shared<UsdMayaSymmetricShaderWriter>(
                depNodeFn,
                usdPath,
                jobCtx,
                usdShaderId);
        });
}

/* static */
UsdMayaShaderWriter::ContextSupport
UsdMayaSymmetricShaderWriter::CanExport(
        const UsdMayaJobExportArgs& exportArgs,
        const TfToken& materialConversionName)
{
    if (materialConversionName.IsEmpty() ||
            exportArgs.convertMaterialsTo == materialConversionName) {
        return ContextSupport::Supported;
    }

    return ContextSupport::Unsupported;
}

UsdMayaSymmetricShaderWriter::UsdMayaSymmetricShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx,
        const TfToken& usdShaderId) :
    UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema =
        UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    shaderSchema.CreateIdAttr(VtValue(usdShaderId));

    for (unsigned int i = 0u; i < depNodeFn.attributeCount(); ++i) {
        const MObject attrObj = depNodeFn.reorderedAttribute(i);
        MPlug attrPlug = depNodeFn.findPlug(attrObj, true);
        const MFnAttribute attrFn(attrObj);

        if (attrPlug.isProcedural() || attrFn.isHidden()) {
            // The Maya docs say these should not be saved off.
            continue;
        }

        if (attrPlug.isChild()) {
            continue;
        }

        // For now, we only support arrays of length 1. If we encounter such an
        // array, we emit it's 0-th element.
        if (attrPlug.isArray()) {
            const unsigned int numElements = attrPlug.evaluateNumElements();
            if (numElements == 0u) {
                // Empty array, so skip this attribute.
                continue;
            }

            if (numElements > 1u) {
                TF_WARN(
                    "Array with multiple elements encountered for "
                    "attribute '%s' on node '%s'. Currently, only arrays "
                    "with a single element are supported.",
                    attrPlug.name().asChar(),
                    UsdMayaUtil::GetMayaNodeName(depNodeFn.object()).c_str());
            }

            attrPlug = attrPlug[0];
        }

        // Keep our authoring sparse by ignoring attributes with no values set
        // and no connections.
        if (!UsdMayaUtil::IsAuthored(attrPlug) && !attrPlug.isConnected()) {
            continue;
        }

        const TfToken usdAttrName = TfToken(
            UsdMayaShadingUtil::GetStandardAttrName(attrPlug, false));
        if (usdAttrName.IsEmpty()) {
            continue;
        }

        const SdfValueTypeName valueTypeName =
            MAYAUSD_NS::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

        // If the Maya attribute is writable, we assume it must be an input.
        // Inputs can still be connected as sources to inputs on other shaders.
        if (attrFn.isWritable()) {
            UsdShadeInput input =
                shaderSchema.CreateInput(usdAttrName, valueTypeName);
            if (!input) {
                continue;
            }

            if (attrPlug.isElement()) {
                UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(), 0u);
            }

            // Add this input to the name/attrPlug map. We'll iterate through
            // these entries during Write() to set their values.
            _inputNameAttrMap.insert(std::make_pair(usdAttrName, attrPlug));
        } else if (attrPlug.isConnected()) {
            // Only author outputs for non-writable attributes if they are
            // connected.
            shaderSchema.CreateOutput(usdAttrName, valueTypeName);
        }
    }
}

/* override */
void
UsdMayaSymmetricShaderWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    for (const auto& inputAttrPair : _inputNameAttrMap) {
        const TfToken& inputName = inputAttrPair.first;
        const MPlug& attrPlug = inputAttrPair.second;

        UsdShadeInput input = shaderSchema.GetInput(inputName);
        if (!input) {
            continue;
        }

        UsdMayaWriteUtil::SetUsdAttr(
            attrPlug,
            input.GetAttr(),
            usdTime,
            _GetSparseValueWriter());
    }
}

/* override */
TfToken
UsdMayaSymmetricShaderWriter::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    UsdShadeShader shaderSchema(_usdPrim);
    if (!shaderSchema) {
        return TfToken();
    }

    // Just check whether we created an input or an attribute with this name,
    // and return its full name if so.

    const UsdShadeInput input = shaderSchema.GetInput(mayaAttrName);
    if (input) {
        return input.GetFullName();
    }

    const UsdShadeOutput output = shaderSchema.GetOutput(mayaAttrName);
    if (output) {
        return output.GetFullName();
    }

    return TfToken();
}


PXR_NAMESPACE_CLOSE_SCOPE
