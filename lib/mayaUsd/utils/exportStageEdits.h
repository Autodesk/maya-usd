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
/// Two files are written:
///   - "edits": for each field that has an in-memory edit, the value composed by the
///     layers of the stage. Only the edited fields are written, not a copy of the
///     layers.
///   - "stage.usda": a wrapper whose sublayers are the edits, then the original files
///     on disk, which the export references instead of the root layer.
///
/// The original files are never modified. Edits of layers that are not part of the
/// layers of the stage (e.g. layers of referenced assets) are not exported: a
/// warning is issued instead.

/// \brief Results of exportStageEdits().
struct ExportedStageEdits
{
    /// Absolute path of the layer to reference instead of the root layer of the
    /// stage. Empty when the stage has no edits to write, or on failure.
    std::string referencedLayer;

    /// Messages about the edits that could not be exported exactly.
    std::vector<std::string> warnings;
};

/// \brief Returns the folder where the edits of the stages are written for an
///        export to the given file: "<export dir>/<export file base name>_edits".
MAYAUSD_CORE_PUBLIC
std::string getStageEditsFolder(const std::string& exportFilePath);

/// \brief Writes the in-memory edits of the stage into \p destFolder, if it has any.
///
/// \param stage The stage to export the edits of.
/// \param destFolder The folder where to write the files. It is created if needed.
/// \param fileFormat The file format of the edits layer: "usda" or "usdc".
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
