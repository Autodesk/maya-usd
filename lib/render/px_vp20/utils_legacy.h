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
#ifndef PXVP20_UTILS_LEGACY_H
#define PXVP20_UTILS_LEGACY_H

/// \file px_vp20/utils_legacy.h

#include "pxr/pxr.h"
#include "../../base/api.h"

#include "pxr/base/gf/matrix4d.h"

#include <maya/MSelectInfo.h>


PXR_NAMESPACE_OPEN_SCOPE


/// This class contains helper methods and utilities to help with the
/// transition from the Maya legacy viewport to Viewport 2.0.
class px_LegacyViewportUtils
{
    public:
        /// Get the view and projection matrices used for selection from the
        /// given selection context in MSelectInfo \p selectInfo.
        MAYAUSD_CORE_PUBLIC
        static bool GetSelectionMatrices(
                MSelectInfo& selectInfo,
                GfMatrix4d& viewMatrix,
                GfMatrix4d& projectionMatrix);

    private:
        px_LegacyViewportUtils() = delete;
        ~px_LegacyViewportUtils() = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE


#endif
