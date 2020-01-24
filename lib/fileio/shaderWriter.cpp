//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "shaderWriter.h"

#include "writeJobContext.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"

#include <maya/MFnDependencyNode.h>


PXR_NAMESPACE_OPEN_SCOPE


UsdMayaShaderWriter::UsdMayaShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
TfToken
UsdMayaShaderWriter::GetShadingPropertyNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    return TfToken();
}

/* virtual */
UsdProperty
UsdMayaShaderWriter::GetShadingPropertyForMayaAttrName(
        const TfToken& mayaAttrName)
{
    const TfToken propertyName =
        GetShadingPropertyNameForMayaAttrName(mayaAttrName);
    if (propertyName.IsEmpty()) {
        return UsdProperty();
    }

    return _usdPrim.GetProperty(propertyName);
}


PXR_NAMESPACE_CLOSE_SCOPE
