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
#ifndef MAYAUSD_PROXYSTAGE_BASE_H
#define MAYAUSD_PROXYSTAGE_BASE_H

#undef MAYAUSD_CORE_PUBLIC
#define MAYAUSD_CORE_PUBLIC

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MObject.h>
#include <maya/MPxSurfaceShape.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase : public MPxSurfaceShape
{
public:
    UsdStageRefPtr getUsdStage() const;
    UsdTimeCode getTime() const;
    static const MString typeName;
    MDagPath parentTransform();

    MayaUsdProxyShapeBase() = default;
    ~MayaUsdProxyShapeBase() override = default;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
