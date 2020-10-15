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
#ifndef PXRUSDMAYA_TRANSLATOR_MODEL_ASSEMBLY_H
#define PXRUSDMAYA_TRANSLATOR_MODEL_ASSEMBLY_H

/// \file usdMaya/translatorModelAssembly.h

#include "usdMaya/api.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MObject.h>

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct UsdMayaTranslatorModelAssembly
{
    /// This method generates a USD prim with a model reference
    /// when provided args and a context that identify a Maya
    /// assembly node.
    PXRUSDMAYA_API
    static bool Create(const UsdMayaPrimWriterArgs& args, UsdMayaPrimWriterContext* context);

    /// This method returns true if \p prim being considered for import under
    /// \p usdImportRootPrim should be imported into Maya as an assembly.
    /// If so, assetIdentifier and assetPrimPath will be populated
    /// appropriately so that they can be used to create an assembly that
    /// references the asset directly if desired.
    /// XXX: This might be a candidate for a plugin point that studios would
    //  want to customize.
    PXRUSDMAYA_API
    static bool ShouldImportAsAssembly(
        const UsdPrim& usdImportRootPrim,
        const UsdPrim& prim,
        std::string*   assetIdentifier,
        SdfPath*       assetPrimPath);

    /// Imports the model at \p prim as a new Maya assembly under
    /// \p parentNode. A USD reference assembly node will be created, and if
    /// \p assemblyRep is non-empty, that representation will be activated
    /// after creation.
    /// Returns true if this succeeds in creating an assembly for \p prim.
    PXRUSDMAYA_API
    static bool Read(
        const UsdPrim&               prim,
        const std::string&           assetIdentifier,
        const SdfPath&               assetPrimPath,
        const MObject&               parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context,
        const TfToken&               assemblyRep);

    /// Creates a Maya USD proxy shape node for the USD prim \p prim under
    /// \p parentNode. A USD proxy shape node will be created.
    /// Returns true if this succeeds in creating a proxy shape for \p prim.
    PXRUSDMAYA_API
    static bool ReadAsProxy(
        const UsdPrim&                            prim,
        const std::map<std::string, std::string>& variantSetSelections,
        MObject                                   parentNode,
        const UsdMayaPrimReaderArgs&              args,
        UsdMayaPrimReaderContext*                 context);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
