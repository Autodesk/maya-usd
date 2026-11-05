//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_EXPORT_STAGE_EDITS_H
#define MAYAUSD_EXPORT_STAGE_EDITS_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/usd/stage.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

/// \file
/// Writes the edits of a USD stage that only exist in memory (the session layer,
/// anonymous layers and the unsaved changes of modified layers) to files, so that
/// a USD export can reference the stage as it is in Maya.
///
/// Only the edits are written: the unsaved changes of a modified layer are written
/// as a delta layer holding the specs and fields that differ from the file on disk.
/// A small wrapper layer then stacks, from strongest to weakest:
///   - a copy of the session layer (with its sublayers flattened),
///   - the anonymous layers and delta layers, in their original strength order,
///   - the original files on disk.
/// The wrapper is what the export references instead of the root layer.
///
/// The original files are never modified. Some edits can't be expressed as a delta
/// and are handled by writing a full copy of the layer, with a warning:
///   - a sublayer that was added, removed, reordered or muted in a layer,
///   - a modified layer that is not part of the root layer stack (e.g. reached
///     through a reference): it is copied and the references to it are repointed.

/// \brief Results of exportStageEdits().
struct ExportedStageEdits
{
    /// Absolute path of the layer to reference instead of the root layer of the
    /// stage. Empty when there was nothing to write, or on failure.
    std::string referencedLayer;

    /// Absolute paths of all the files that were written.
    std::vector<std::string> writtenFiles;

    /// Identifiers of the file-backed layers that had to be written in full
    /// (see the fallback cases above).
    std::vector<std::string> copiedLayers;

    /// Messages about the edits that could not be exported exactly.
    std::vector<std::string> warnings;
};

/// \brief Returns the folder where the edits of the stages are written for an
///        export to the given file: "<export dir>/<export file base name>_edits".
MAYAUSD_CORE_PUBLIC
std::string getStageEditsFolder(const std::string& exportFilePath);

/// \brief Returns true if the stage has edits that are not in the files on disk:
///        an anonymous root layer, modified or anonymous layers, muted sublayers,
///        or opinions in the session layer (ignoring the Maya-USD bookkeeping of
///        prims edited as Maya).
MAYAUSD_CORE_PUBLIC
bool stageHasUnsavedEdits(const PXR_NS::UsdStageRefPtr& stage);

/// \brief Writes the in-memory edits of the stage into \p destFolder.
///
/// \param stage The stage to export the edits of.
/// \param destFolder The folder where to write the files. It is created if needed.
/// \param fileFormat The file format of the written layers: "usda" or "usdc".
/// \param relativeOriginalPaths When true, the paths to the original files are
///        written relative to \p destFolder, otherwise as absolute paths.
MAYAUSD_CORE_PUBLIC
ExportedStageEdits exportStageEdits(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            destFolder,
    const std::string&            fileFormat,
    bool                          relativeOriginalPaths);

} // namespace MAYAUSD_NS_DEF

#endif
