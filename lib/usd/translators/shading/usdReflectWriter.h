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
#ifndef PXRUSDTRANSLATORS_REFLECT_WRITER_H
#define PXRUSDTRANSLATORS_REFLECT_WRITER_H

/// \file

#include "usdLambertWriter.h"

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for exporting the reflective part of a Maya shading node to USD.
/// Will be used by Blinn, Phong, and PhongE to export common specular attributes.
class PxrUsdTranslators_ReflectWriter : public PxrUsdTranslators_LambertWriter
{
    typedef PxrUsdTranslators_LambertWriter BaseClass;

public:
    PxrUsdTranslators_ReflectWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

protected:
    void WriteSpecular(const UsdTimeCode& usdTime) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
