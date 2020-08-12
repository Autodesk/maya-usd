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
#ifndef PXRUSDTRANSLATORS_LAMBERT_READER_H
#define PXRUSDTRANSLATORS_LAMBERT_READER_H

/// \file

#include "usdMaterialReader.h"

#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for importing UsdPreviewSurface to Maya's lambert material nodes
class PxrUsdTranslators_LambertReader : public PxrUsdTranslators_MaterialReader {
    typedef PxrUsdTranslators_MaterialReader _BaseClass;

public:
    PxrUsdTranslators_LambertReader(const UsdMayaPrimReaderArgs&);

    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    virtual TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const;

protected:
    const TfToken& _GetMayaNodeTypeName() const override;

    void _OnReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
