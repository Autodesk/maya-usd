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
#ifndef USDUFE_STAGE_EDITS_H
#define USDUFE_STAGE_EDITS_H

#include <usdUfe/base/api.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <functional>
#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

/// \file
/// Extracts the edits of a USD stage that only exist in memory (the session layer,
/// anonymous layers and the unsaved changes of modified layers) into a single layer,
/// e.g. to export or reference the stage as it is in an application.
///
/// The layer holds, for each field that has an in-memory edit, the value composed by
/// the layers of the stage, and has the original files of the stage as sublayers.
/// Only the edited fields are written, not a copy of the layers: opening that layer
/// gives the same result as the stage. The original files are never modified.
///
/// Edits of layers that are not part of the layers of the stage (e.g. layers of
/// referenced assets) are not extracted: a warning is issued instead.

/// \brief Options of extractStageEdits().
struct StageEditsOptions
{
    /// Where the layer will be saved. The asset paths of the edits and the paths to
    /// the original files are written for this location.
    std::string layerPath;

    /// When true, the paths to the original files are relative to the folder of the
    /// layer, otherwise they are absolute.
    bool relativeOriginalPaths = false;

    /// Called on a copy of the session layer before its edits are extracted, to remove
    /// the opinions that are only meaningful in the application.
    std::function<void(const PXR_NS::SdfLayerHandle&)> sessionLayerFilter;
};

/// \brief Results of extractStageEdits().
struct StageEdits
{
    /// The anonymous layer holding the edits, with the original files of the stage as
    /// sublayers. Null when the stage has no edits: it can be used as is.
    PXR_NS::SdfLayerRefPtr layer;

    /// Messages about the edits that could not be extracted exactly.
    std::vector<std::string> warnings;
};

/// \brief Extracts the in-memory edits of the stage into a layer, if it has any.
USDUFE_PUBLIC
StageEdits extractStageEdits(const PXR_NS::UsdStageRefPtr& stage, const StageEditsOptions& options);

} // namespace USDUFE_NS_DEF

#endif
