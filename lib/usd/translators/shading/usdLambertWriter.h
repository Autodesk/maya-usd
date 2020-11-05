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
#ifndef PXRUSDTRANSLATORS_LAMBERT_WRITER_H
#define PXRUSDTRANSLATORS_LAMBERT_WRITER_H

/// \file

#include "usdMaterialWriter.h"

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for exporting the Lambert part of a Maya shading node to USD.
/// Will be used by Blinn, Phong, and PhongE to export the Lambertian attributes.
class PxrUsdTranslators_LambertWriter : public PxrUsdTranslators_MaterialWriter
{
    typedef PxrUsdTranslators_MaterialWriter BaseClass;

public:
    PxrUsdTranslators_LambertWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

protected:
    virtual void WriteSpecular(const UsdTimeCode& usdTime);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
