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
#ifndef PXRUSDMAYA_TRANSLATOR_PRIM_H
#define PXRUSDMAYA_TRANSLATOR_PRIM_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for reading UsdPrim.  This should more
/// accurately take a UsdGeomImageable.
struct UsdMayaTranslatorPrim
{
    MAYAUSD_CORE_PUBLIC
    static void Read(
        const UsdPrim&               prim,
        MObject                      mayaNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
