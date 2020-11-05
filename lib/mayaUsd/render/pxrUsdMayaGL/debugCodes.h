//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_DEBUG_CODES
#define PXRUSDMAYAGL_DEBUG_CODES

/// \file pxrUsdMayaGL/debugCodes.h

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    PXRUSDMAYAGL_BATCHED_DRAWING,
    PXRUSDMAYAGL_BATCHED_SELECTION,
    PXRUSDMAYAGL_INSTANCER_TRACKING,
    PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING,
    PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
