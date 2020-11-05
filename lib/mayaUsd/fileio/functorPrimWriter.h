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
#ifndef PXRUSDMAYA_FUNCTOR_PRIM_WRITER_H
#define PXRUSDMAYA_FUNCTOR_PRIM_WRITER_H

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/transformWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMaya_FunctorPrimWriter
/// \brief This class is scaffolding to hold bare prim writer functions and
/// adapt them to the UsdMayaPrimWriter or UsdMayaTransformWriter interface
/// (depending on whether the writer plugin is handling a shape or a transform).
///
/// It is used by the PXRUSDMAYA_DEFINE_WRITER macro.
class UsdMaya_FunctorPrimWriter final : public UsdMayaTransformWriter
{
public:
    UsdMaya_FunctorPrimWriter(
        const MFnDependencyNode&            depNodeFn,
        const SdfPath&                      usdPath,
        UsdMayaWriteJobContext&             jobCtx,
        UsdMayaPrimWriterRegistry::WriterFn plugFn);

    ~UsdMaya_FunctorPrimWriter() override;

    void                 Write(const UsdTimeCode& usdTime) override;
    bool                 ExportsGprims() const override;
    bool                 ShouldPruneChildren() const override;
    const SdfPathVector& GetModelPaths() const override;

    static UsdMayaPrimWriterSharedPtr Create(
        const MFnDependencyNode&            depNodeFn,
        const SdfPath&                      usdPath,
        UsdMayaWriteJobContext&             jobCtx,
        UsdMayaPrimWriterRegistry::WriterFn plugFn);

    static UsdMayaPrimWriterRegistry::WriterFactoryFn
    CreateFactory(UsdMayaPrimWriterRegistry::WriterFn plugFn);

private:
    UsdMayaPrimWriterRegistry::WriterFn _plugFn;
    bool                                _exportsGprims;
    bool                                _pruneChildren;
    SdfPathVector                       _modelPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
