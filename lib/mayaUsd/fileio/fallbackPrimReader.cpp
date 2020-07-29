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
#include "fallbackPrimReader.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/usd/usdGeom/imageable.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_FallbackPrimReader::UsdMaya_FallbackPrimReader(const UsdMayaPrimReaderArgs& args)
    : UsdMayaPrimReader(args)
{
}

bool UsdMaya_FallbackPrimReader::Read(UsdMayaPrimReaderContext* context)
{
    const UsdPrim& usdPrim = _GetArgs().GetUsdPrim();
    if (usdPrim.HasAuthoredTypeName() && !usdPrim.IsA<UsdGeomImageable>()) {
        // Only create fallback Maya nodes for untyped prims or imageable prims
        // that have no prim reader.
        return false;
    }

    MObject parentNode = context->GetMayaNode(usdPrim.GetPath().GetParentPath(), true);

    MStatus status;
    MObject mayaNode;
    return UsdMayaTranslatorUtil::CreateDummyTransformNode(
        usdPrim,
        parentNode,
        /*importTypeName*/ false,
        _GetArgs(),
        context,
        &status,
        &mayaNode);
}

/* static */
UsdMayaPrimReaderRegistry::ReaderFactoryFn UsdMaya_FallbackPrimReader::CreateFactory()
{
    return [](const UsdMayaPrimReaderArgs& args) {
        return std::make_shared<UsdMaya_FallbackPrimReader>(args);
    };
}

PXR_NAMESPACE_CLOSE_SCOPE
