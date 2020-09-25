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

#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/rfmShaderMap.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((conversionName, "rendermanForMaya"))
    ((renderContext, "ri"))
    ((niceName, "RenderMan for Maya"))
    ((description, "Exports bound shaders as a RenderMan for Maya UsdShade network."))
);


REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    _tokens->conversionName,
    _tokens->renderContext,
    _tokens->niceName,
    _tokens->description);


// Register a symmetric shader writer for each Maya node type name and USD
// shader ID mapping. These writers will only apply when the applicable
// material conversion is requested.
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    for (const auto& i : RfmNodesToShaderIds) {
        PxrUsdTranslators_SymmetricShaderWriter::RegisterWriter(
            i.first,
            i.second,
            _tokens->conversionName);
    }
};


PXR_NAMESPACE_CLOSE_SCOPE
