//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
            const std::string& iFileName,
            const std::string& iPrimPath,
            const std::map<std::string, std::string>& iVariants,
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
