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
#ifndef PXRUSDTRANSLATORS_PARTICLE_WRITER_H
#define PXRUSDTRANSLATORS_PARTICLE_WRITER_H

/// \file

#include <mayaUsd/fileio/transformWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/points.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MString.h>

#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_ParticleWriter : public UsdMayaTransformWriter
{
public:
    PxrUsdTranslators_ParticleWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

private:
    void writeParams(const UsdTimeCode& usdTime, UsdGeomPoints& points);

    enum ParticleType
    {
        PER_PARTICLE_INT,
        PER_PARTICLE_DOUBLE,
        PER_PARTICLE_VECTOR
    };

    std::vector<std::tuple<TfToken, MString, ParticleType>> mUserAttributes;
    bool                                                    mInitialFrameDone;

    void initializeUserAttributes();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
