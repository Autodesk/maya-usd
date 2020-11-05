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
#ifndef PXRUSDMAYA_PRIMWRITERCONTEXT_H
#define PXRUSDMAYA_PRIMWRITERCONTEXT_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimWriterContext
/// \brief This class provides an interface for writer plugins to communicate
/// state back to the core usd maya logic.
class UsdMayaPrimWriterContext
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimWriterContext(
        const UsdTimeCode&    timeCode,
        const SdfPath&        authorPath,
        const UsdStageRefPtr& stage);

    /// \brief returns the time frame where data should be authored.
    MAYAUSD_CORE_PUBLIC
    const UsdTimeCode& GetTimeCode() const;

    /// \brief returns the path where the writer plugin should create
    /// a prim.
    MAYAUSD_CORE_PUBLIC
    const SdfPath& GetAuthorPath() const;

    /// \brief returns the usd stage that is being written to.
    MAYAUSD_CORE_PUBLIC
    UsdStageRefPtr GetUsdStage() const;

    /// \brief Returns the value provided by SetExportsGprims(), or \c false
    /// if SetExportsGprims() is not called.
    ///
    /// May be used by export processes to reason about what kind of asset we
    /// are creating.
    MAYAUSD_CORE_PUBLIC
    bool GetExportsGprims() const;

    /// Set the value that will be returned by GetExportsGprims().
    ///
    /// A plugin should set this to \c true if it directly creates any
    /// gprims, and should return the same value each time its write()
    /// function is invoked.
    ///
    /// \sa GetExportsGprims()
    MAYAUSD_CORE_PUBLIC
    void SetExportsGprims(bool exportsGprims);

    /// Set the value that will be returned by GetPruneChildren().
    ///
    /// A plugin should set this to \c true if it will handle writing
    /// child prims by itself, or if it does not wish for any children of
    /// the current node to be traversed by the export process.
    ///
    /// This should be called during the initial (unvarying) export for it
    /// to be considered by the export process. If it is called during the
    /// animated (varying) export, it will be ignored.
    MAYAUSD_CORE_PUBLIC
    void SetPruneChildren(bool pruneChildren);

    /// \brief Returns the value provided by SetPruneChildren(), or \c false
    /// if SetPruneChildren() is not called.
    ///
    /// Export processes should prune all descendants of the current node
    /// during traversal if this is set to \c true.
    MAYAUSD_CORE_PUBLIC
    bool GetPruneChildren() const;

    /// Gets the value provided by SetModelPaths().
    /// The default value is an empty vector if SetModelPaths() was
    /// never called.
    MAYAUSD_CORE_PUBLIC
    const SdfPathVector& GetModelPaths() const;

    /// Sets the vector of prim paths that the prim writer declares as
    /// potentially being models. These are prims on which this prim writer has
    /// authored kind metadata or otherwise expects kind metadata to exist
    /// (e.g. via reference).
    ///
    /// The USD export process will attempt to "fix-up" kind metadata to ensure
    /// contiguous model hierarchy.
    MAYAUSD_CORE_PUBLIC
    void SetModelPaths(const SdfPathVector& modelPaths);

    /// Overload of SetModelPaths() that moves the input vector.
    MAYAUSD_CORE_PUBLIC
    void SetModelPaths(SdfPathVector&& modelPaths);

private:
    const UsdTimeCode& _timeCode;
    const SdfPath&     _authorPath;
    UsdStageRefPtr     _stage;
    bool               _exportsGprims;
    bool               _pruneChildren;
    SdfPathVector      _modelPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
