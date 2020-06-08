//
// Copyright 2017 Animal Logic
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
#pragma once

#include <pxr/pxr.h>
#include <pxr/base/tf/debug.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(

    ALUSDMAYA_TRANSLATORS,
    ALUSDMAYA_EVENTS,
    ALUSDMAYA_EVALUATION,
    ALUSDMAYA_COMMANDS,
    ALUSDMAYA_LAYERS,
    ALUSDMAYA_DRAW,
    ALUSDMAYA_SELECTION,
    ALUSDMAYA_GUIHELPER,
    ALUSDMAYA_RENDERER,
    ALUSDMAYA_GEOMETRY_DEFORMER,
    ALUSDMAYA_TRANSFORM_MATRIX

);

PXR_NAMESPACE_CLOSE_SCOPE

namespace AL {
namespace maya {

using PXR_NS::TfDebug;

} // maya
} // AL

