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
#ifndef PXRUSDTRANSLATORS_NURBS_CURVE_WRITER_H
#define PXRUSDTRANSLATORS_NURBS_CURVE_WRITER_H

/// \file pxrUsdTranslators/nurbsCurveWriter.h

#include "pxr/pxr.h"

#include "../../fileio/primWriter.h"
#include "../../fileio/writeJobContext.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"

#include <maya/MFnDependencyNode.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Exports Maya nurbsCurve objects (MFnNurbsCurve) as UsdGeomNurbsCurves.
class PxrUsdTranslators_NurbsCurveWriter : public UsdMayaPrimWriter
{
    public:
        PxrUsdTranslators_NurbsCurveWriter(
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx);

        void Write(const UsdTimeCode& usdTime) override;

        bool ExportsGprims() const override;

    protected:
        bool writeNurbsCurveAttrs(
                const UsdTimeCode& usdTime,
                UsdGeomNurbsCurves& primSchema);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
