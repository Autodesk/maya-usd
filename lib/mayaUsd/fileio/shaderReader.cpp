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
#include "shaderReader.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaShaderReader::UsdMayaShaderReader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaPrimReader(readArgs)
{
}

/* virtual */
TfToken UsdMayaShaderReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
