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
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderReader.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((conversionName, "maya"))
    ((renderContext, "maya"))
    ((niceName, "Maya Shaders"))
    ((exportDescription,
        "Dumps the bound shader in a Maya UsdShade network that can only be "
        "used for import. Will not render in the Maya viewport or usdView."))
    ((importDescription,
        "Fetches back a Maya shader network dumped as UsdShade"))
);

REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    _tokens->conversionName,
    _tokens->renderContext,
    _tokens->niceName,
    _tokens->exportDescription);

REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION(
    _tokens->conversionName,
    _tokens->renderContext,
    _tokens->niceName,
    _tokens->importDescription);

namespace {

template<typename F>
void _RegisterMayaNodes(F _registryFunction)
{
    // All dependency nodes with a "drawdb/shader" classification are supported.
    const MString     nodeTypesCmd("stringArrayToString(listNodeTypes(\"drawdb/shader\"), \" \");");
    const std::string cmdResult = MGlobal::executeCommandStringResult(nodeTypesCmd).asChar();
    std::vector<std::string> mayaNodeTypeNames = TfStringTokenize(cmdResult);

    // The "place3dTexture" node which has classification "drawdb/geometry" is
    // also supported.
    mayaNodeTypeNames.push_back("place3dTexture");

    for (const std::string& mayaNodeTypeName : mayaNodeTypeNames) {
        const TfToken nodeTypeNameToken(mayaNodeTypeName);

        _registryFunction(nodeTypeNameToken, nodeTypeNameToken, _tokens->conversionName);
    }
}
} // namespace

TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    _RegisterMayaNodes(UsdMayaSymmetricShaderWriter::RegisterWriter);
}

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    _RegisterMayaNodes(UsdMayaSymmetricShaderReader::RegisterReader);
}

PXR_NAMESPACE_CLOSE_SCOPE
