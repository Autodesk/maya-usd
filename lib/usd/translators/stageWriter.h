//
// Copyright 2024 Autodesk
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
#ifndef PXRUSDTRANSLATORS_STAGE_WRITER_H
#define PXRUSDTRANSLATORS_STAGE_WRITER_H

/// \file

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase;

/// Exports USD stage proxy shapes to a prim containing a USD reference.
class PxrUsdTranslators_StageWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_StageWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    void PostExport() override;

private:
    // When the stage has edits that are only in memory and the export options ask for
    // them, writes them next to the exported file. Returns true and the path of the
    // layer to reference if they were written.
    bool exportStageEdits(
        MayaUsdProxyShapeBase& proxyShape,
        const UsdStageRefPtr&  proxyStage,
        std::string&           refIdentifier);

    // Warns when other writers (e.g. the Maya materials scope) defined prims at the
    // same paths as the references to the root prims of the stage.
    void warnAboutNameClashes(const UsdStage& exportedStage) const;

    std::vector<TfToken> _referencedRootNames;
    bool                 _checkedStageEdits = false;  // The edits were looked for.
    bool                 _exportedStageEdits = false; // The edits were written.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
