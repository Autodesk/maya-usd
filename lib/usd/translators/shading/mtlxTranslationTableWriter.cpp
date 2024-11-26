//
// Copyright 2024 Autodesk
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

#include "mtlxTranslationTableWriter.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

MtlxUsd_TranslationTableWriter::MtlxUsd_TranslationTableWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx,
    TfToken                  materialName,
    const TranslationTable&  translationTable,
    const AlwaysAuthored&    alwaysAuthored)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
    , _materialName(materialName)
    , _translationTable(translationTable)
    , _alwaysAuthored(alwaysAuthored)
{
    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
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

    shaderSchema.CreateIdAttr(VtValue(_materialName));

    UsdShadeNodeGraph nodegraphSchema;

    for (unsigned int i = 0u; i < depNodeFn.attributeCount(); ++i) {
        const MObject      attrObj = depNodeFn.reorderedAttribute(i);
        const MFnAttribute attrFn(attrObj);

        const TfToken                    mayaAttrName = TfToken(attrFn.name().asChar());
        TranslationTable::const_iterator renaming = _translationTable.find(mayaAttrName);

        if (renaming == _translationTable.cend()) {
            continue;
        }

        MPlug attrPlug = depNodeFn.findPlug(attrObj, true);

        const TfToken usdAttrName = renaming->second;

        // Keep our authoring sparse by ignoring attributes with no values set and no connections.
        // Some attributes with an history of default value updates will be written always.
        if (!(UsdMayaUtil::IsAuthored(attrPlug) || _alwaysAuthored.count(usdAttrName))
            && !attrPlug.isConnected() && !attrPlug.numConnectedChildren()) {
            continue;
        }

        const SdfValueTypeName valueTypeName = MayaUsd::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

        UsdShadeInput input = shaderSchema.CreateInput(usdAttrName, valueTypeName);
        if (!input) {
            continue;
        }

        if (attrPlug.isElement()) {
            UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(), 0u);
        }

        // Add this input to the name/attrPlug map. We'll iterate through
        // these entries during Write() to set their values.
        _inputNameAttrMap.insert(std::make_pair(usdAttrName, attrPlug));

        // All connections go directly to the node graph:
        if (attrPlug.isConnected() || attrPlug.numConnectedChildren()) {
            if (!nodegraphSchema) {
                nodegraphSchema = UsdShadeNodeGraph(GetNodeGraph());
                if (!TF_VERIFY(
                        nodegraphSchema,
                        "Could not define UsdShadeNodeGraph at path '%s'\n",
                        GetUsdPath().GetText())) {
                    return;
                }
            }
            UsdShadeOutput ngOutput = nodegraphSchema.CreateOutput(mayaAttrName, valueTypeName);
            input.ConnectToSource(ngOutput);
        }
    }

    // Surface Output
    shaderSchema.CreateOutput(_GetOutputName(_materialName), SdfValueTypeNames->Token);
}

/* override */
void MtlxUsd_TranslationTableWriter::Write(const UsdTimeCode& usdTime)
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
        const MPlug&   attrPlug = inputAttrPair.second;

        UsdShadeInput input = shaderSchema.GetInput(inputName);
        if (!input || attrPlug.isConnected() || attrPlug.numConnectedChildren()) {
            continue;
        }

        // Color values are all linear on the shader, so do not re-linearize
        // them.
        VtValue value = UsdMayaWriteUtil::GetVtValue(
            attrPlug,
            MayaUsd::Converter::getUsdTypeName(attrPlug),
            /* linearizeColors = */ false);

        input.Set(value, usdTime);
    }
}

/* override */
UsdAttribute MtlxUsd_TranslationTableWriter::GetShadingAttributeForMayaAttrName(
    const TfToken& mayaAttrName,
    const SdfValueTypeName&)
{
    if (mayaAttrName == TrMayaTokens->outColor) {
        UsdShadeShader surfaceSchema(_usdPrim);
        if (!surfaceSchema) {
            return UsdAttribute();
        }

        // Surface output is on the shader itself
        return surfaceSchema.GetOutput(_GetOutputName(_materialName));
    }

    // All other are outputs of the NodeGraph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!nodegraphSchema) {
        return UsdAttribute();
    }

    if (mayaAttrName == TrMayaTokens->normalCamera || mayaAttrName == TrMayaTokens->coatNormal) {
        // Add the proper nodes for normal mapping:
        return AddNormalMapping(nodegraphSchema.GetOutput(mayaAttrName));
    }

    // And they use the camelCase Maya name directly:
    UsdShadeOutput output = nodegraphSchema.GetOutput(mayaAttrName);
    if (output) {
        return output;
    }

    // We did not find the attribute directly, but we might be dealing with a subcomponent
    // connection on a compound attribute:
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);

    MPlug childPlug = depNodeFn.findPlug(mayaAttrName.GetText(), &status);
    if (!status || childPlug.isNull() || !childPlug.isChild()) {
        return {};
    }

    MPlug              parentPlug = childPlug.parent();
    unsigned int       childIndex = 0;
    const unsigned int numChildren = parentPlug.numChildren();
    for (; childIndex < numChildren; ++childIndex) {
        if (childPlug.attribute() == parentPlug.child(childIndex).attribute()) {
            break;
        }
    }

    // We need the long name of the attribute:
    const TfToken parentAttrName(
        parentPlug.partialName(false, false, false, false, false, true).asChar());
    output = nodegraphSchema.GetOutput(parentAttrName);
    if (output) {
        return AddConstructor(output, static_cast<size_t>(childIndex), parentPlug);
    }

    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
