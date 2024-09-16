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
#ifndef PXRUSDMAYA_READ_JOB_H
#define PXRUSDMAYA_READ_JOB_H

#include <mayaUsd/fileio/chaser/importChaser.h>
#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaPrimReaderArgs;

class UsdMaya_ReadJob
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMaya_ReadJob(const MayaUsd::ImportData& iImportData, const UsdMayaJobImportArgs& iArgs);

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMaya_ReadJob();

    /// Reads the USD stage specified by the job file name and prim path.
    /// Newly-created DAG paths are inserted into the vector \p addedDagPaths.
    MAYAUSD_CORE_PUBLIC
    bool Read(std::vector<MDagPath>* addedDagPaths);

    /// Redoes a previous Read() operation after Undo() has been called.
    /// If Undo() hasn't been called, does nothing.
    MAYAUSD_CORE_PUBLIC
    bool Redo();

    /// Undoes a previous Read() operation, removing all added nodes.
    MAYAUSD_CORE_PUBLIC
    bool Undo();

    // Getters/Setters
    MAYAUSD_CORE_PUBLIC
    void SetMayaRootDagPath(const MDagPath& mayaRootDagPath);
    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetMayaRootDagPath() const;

    MAYAUSD_CORE_PUBLIC
    double timeSampleMultiplier() const;

    MAYAUSD_CORE_PUBLIC
    const UsdMayaPrimReaderContext::ObjectRegistry& GetNewNodeRegistry() const;

protected:
    // Types
    using _PrimReaderMap = std::unordered_map<SdfPath, UsdMayaPrimReaderSharedPtr, SdfPath::Hash>;

    MAYAUSD_CORE_PUBLIC
    virtual bool DoImport(UsdPrimRange& range, const UsdPrim& usdRootPrim);

    // Hook for derived classes to override the prim reader.  Returns true if
    // override was done, false otherwise.  Implementation in this class
    // returns false.
    MAYAUSD_CORE_PUBLIC
    virtual bool OverridePrimReader(
        const UsdPrim&               usdRootPrim,
        const UsdPrim&               prim,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext&    readCtx,
        UsdPrimRange::iterator&      primIt);

    // Engine method for DoImport().  Covers the functionality of a regular
    // usdImport.
    MAYAUSD_CORE_PUBLIC
    bool _DoImport(UsdPrimRange& range, const UsdPrim& usdRootPrim);

    // Hook for derived classes to perform processing before import.
    // Method in this class is a no-op.
    MAYAUSD_CORE_PUBLIC
    virtual void PreImport(Usd_PrimFlagsPredicate& returnPredicate);

    // Hook for derived classes to determine whether to skip the root prim
    // on prim traversal.  This class returns the argument unchanged.
    MAYAUSD_CORE_PUBLIC
    virtual bool SkipRootPrim(bool isImportingPseudoRoot);

    // Data
    UsdMayaJobImportArgs                     mArgs;
    const MayaUsd::ImportData&               mImportData;
    UsdMayaPrimReaderContext::ObjectRegistry mNewNodeRegistry;
    MDagPath                                 mMayaRootDagPath;

private:
    void _DoImportPrimIt(
        UsdPrimRange::iterator&   primIt,
        const UsdPrim&            usdRootPrim,
        UsdMayaPrimReaderContext& readCtx,
        _PrimReaderMap&           primReaders);

    void _DoImportInstanceIt(
        UsdPrimRange::iterator&   primIt,
        const UsdPrim&            usdRootPrim,
        UsdMayaPrimReaderContext& readCtx,
        _PrimReaderMap&           primReaders);

    void _ImportPrototype(
        const UsdPrim&            prototype,
        const UsdPrim&            usdRootPrim,
        UsdMayaPrimReaderContext& readCtx);

    double _setTimeSampleMultiplierFrom(const double layerFPS);

    // Data
    MDagModifier mDagModifierUndo;
    bool         mDagModifierSeeded;
    double       mTimeSampleMultiplier;

    /// Cache of import chasers that were run. Currently used to aid in redo/undo operations
    /// This cache is cleared for every new Read() operation.
    UsdMayaImportChaserRefPtrVector mImportChasers;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
