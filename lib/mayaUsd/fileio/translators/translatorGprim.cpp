//
// Copyright 2016 Pixar
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
#include "translatorGprim.h"

#include <mayaUsd/utils/util.h>

#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE

void UsdMayaTranslatorGprim::Read(
    const UsdGeomGprim& gprim,
    MObject             mayaNode,
    UsdMayaPrimReaderContext*)
{
    MFnDagNode fnGprim(mayaNode);

    bool doubleSided;
    if (gprim.GetDoubleSidedAttr().Get(&doubleSided)) {
        UsdMayaUtil::setPlugValue(fnGprim, "doubleSided", doubleSided);
    }
}

void UsdMayaTranslatorGprim::Write(
    const MObject&      mayaNode,
    const UsdGeomGprim& gprim,
    UsdMayaPrimWriterContext*,
    GeomSidedness sidedness)
{
    MFnDependencyNode depFn(mayaNode);

    bool doubleSided = sidedness == GeomSidedness::Double;
    if (sidedness == GeomSidedness::Derived) {
        if (UsdMayaUtil::getPlugValue(depFn, "doubleSided", &doubleSided)) {
            gprim.CreateDoubleSidedAttr(VtValue(doubleSided), true);
        }
    } else {
        gprim.CreateDoubleSidedAttr(VtValue(doubleSided), true);
    }

    bool opposite = false;
    // Gprim properties always authored on the shape
    if (UsdMayaUtil::getPlugValue(depFn, "opposite", &opposite)) {
        // If mesh is double sided in maya, opposite is disregarded
        TfToken orientation
            = (opposite && !doubleSided ? UsdGeomTokens->leftHanded : UsdGeomTokens->rightHanded);
        gprim.CreateOrientationAttr(VtValue(orientation), true);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
