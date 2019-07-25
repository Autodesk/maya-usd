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
#include "usdMaya/readJobWithSceneAssembly.h"
#include "usdMaya/translatorModelAssembly.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_ReadJobWithSceneAssembly::UsdMaya_ReadJobWithSceneAssembly(
        const std::string &iFileName,
        const std::string &iPrimPath,
        const std::map<std::string, std::string>& iVariants,
        const UsdMayaJobImportArgs &iArgs) :
    UsdMaya_ReadJob(iFileName, iPrimPath, iVariants, iArgs)
{
}

UsdMaya_ReadJobWithSceneAssembly::~UsdMaya_ReadJobWithSceneAssembly()
{
}

bool
UsdMaya_ReadJobWithSceneAssembly::DoImport(UsdPrimRange& rootRange, const UsdPrim& usdRootPrim)
{
    return mArgs.importWithProxyShapes ?
        _DoImportWithProxies(rootRange) : _DoImport(rootRange, usdRootPrim);
}

bool UsdMaya_ReadJobWithSceneAssembly::OverridePrimReader(
    const UsdPrim&               usdRootPrim,
    const UsdPrim&               prim,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext&    readCtx,
    UsdPrimRange::iterator&      primIt
)
{
    // If we are NOT importing on behalf of an assembly, then we'll
    // create reference assembly nodes that target the asset file
    // and the root prims of those assets directly. This ensures
    // that a re-export will work correctly, since USD references
    // can only target root prims.
    std::string assetIdentifier;
    SdfPath assetPrimPath;
    if (UsdMayaTranslatorModelAssembly::ShouldImportAsAssembly(
            usdRootPrim,
            prim,
            &assetIdentifier,
            &assetPrimPath)) {
        const bool isSceneAssembly =
            GetMayaRootDagPath().node().hasFn(MFn::kAssembly);
        if (isSceneAssembly) {
            // If we ARE importing on behalf of an assembly, we use
            // the file path of the top-level assembly and the path
            // to the prim within that file when creating the
            // reference assembly.
            assetIdentifier = mFileName;
            assetPrimPath = prim.GetPath();
        }

        // Note that if assemblyRep == "Import", the assembly reader
        // will NOT run and we will fall through to the prim reader.
        MObject parentNode = readCtx.GetMayaNode(
            prim.GetPath().GetParentPath(), false);
        if (UsdMayaTranslatorModelAssembly::Read(
                prim,
                assetIdentifier,
                assetPrimPath,
                parentNode,
                args,
                &readCtx,
                mArgs.assemblyRep)) {
            if (readCtx.GetPruneChildren()) {
                primIt.PruneChildren();
            }
            return true;
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
