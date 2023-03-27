//
// Copyright 2019 Luma Pictures
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
#ifndef MAYAHYDRALIB_ADAPTER_DEBUG_CODES_H
#define MAYAHYDRALIB_ADAPTER_DEBUG_CODES_H

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

//! \brief  Some variables to enable debug printing information for adapters

// clang-format off
TF_DEBUG_CODES(
    MAYAHYDRALIB_ADAPTER_CALLBACKS,
    MAYAHYDRALIB_ADAPTER_CURVE_PLUG_DIRTY,
    MAYAHYDRALIB_ADAPTER_CURVE_UNHANDLED_PLUG_DIRTY,
    MAYAHYDRALIB_ADAPTER_DAG_HIERARCHY,
    MAYAHYDRALIB_ADAPTER_DAG_PLUG_DIRTY,
    MAYAHYDRALIB_ADAPTER_GET,
    MAYAHYDRALIB_ADAPTER_GET_LIGHT_PARAM_VALUE,
    MAYAHYDRALIB_ADAPTER_IMAGEPLANES,
    MAYAHYDRALIB_ADAPTER_LIGHT_SHADOWS,
    MAYAHYDRALIB_ADAPTER_MATERIALS,
    MAYAHYDRALIB_ADAPTER_MESH_PLUG_DIRTY,
    MAYAHYDRALIB_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY,
    MAYAHYDRALIB_ADAPTER_MATERIALS_PRINT_PARAMETERS_VALUES
    );
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_ADAPTER_DEBUG_CODES_H
