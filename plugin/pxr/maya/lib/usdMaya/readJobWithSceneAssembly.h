//
// Copyright 2019 Pixar
// Copyright 2019 Autodesk
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
#ifndef PXRUSDMAYA_READ_JOB_WITH_SCENE_ASSEMBLY_H
#define PXRUSDMAYA_READ_JOB_WITH_SCENE_ASSEMBLY_H

/// \file usdMaya/readJobWithSceneAssembly.h

#include <mayaUsd/fileio/jobs/readJob.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMaya_ReadJobWithSceneAssembly : public UsdMaya_ReadJob
{
public:
    UsdMaya_ReadJobWithSceneAssembly(
            const MayaUsd::ImportData& iImportData,
            const UsdMayaJobImportArgs & iArgs);

    ~UsdMaya_ReadJobWithSceneAssembly();

private:

    bool DoImport(UsdPrimRange& range, const UsdPrim& usdRootPrim) override;
    bool OverridePrimReader(
        const UsdPrim&               usdRootPrim,
        const UsdPrim&               prim,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext&    readCtx,
        UsdPrimRange::iterator&      primIt
    ) override;

    // Hook to set the shading mode if dealing with scene assembly.
    void PreImport(Usd_PrimFlagsPredicate& returnPredicate) override;

    bool SkipRootPrim(bool isImportingPseudoRoot) override;

    // XXX: Activating the 'Expanded' representation of a USD reference assembly
    // node is very much like performing a regular UsdMaya_ReadJob but with
    // a few key differences (e.g. creating proxy shapes at collapse points).
    // This private helper method covers the functionality of an 'Expanded'
    // representation-style import.  It would be great if we could combine
    // these into a single traversal at some point.
    bool _DoImportWithProxies(UsdPrimRange& range);

    // These are helper methods for the proxy import method.
    bool _ProcessProxyPrims(
            const std::vector<UsdPrim>& proxyPrims,
            const UsdPrim& pxrGeomRoot,
            const std::vector<std::string>& collapsePointPathStrings);
    bool _ProcessSubAssemblyPrims(const std::vector<UsdPrim>& subAssemblyPrims);
    bool _ProcessCameraPrims(const std::vector<UsdPrim>& cameraPrims);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
