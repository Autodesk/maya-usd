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

/// \file usdMaya/readJob.h

#include "jobArgs.h"
#include "../primReaderContext.h"

#include "pxr/pxr.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"

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
    UsdMaya_ReadJob(
            const std::string& iFileName,
            const std::string& iPrimPath,
            const std::map<std::string, std::string>& iVariants,
            const UsdMayaJobImportArgs & iArgs);

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
    void SetMayaRootDagPath(const MDagPath &mayaRootDagPath);
    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetMayaRootDagPath() const;

protected:

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
        UsdPrimRange::iterator&      primIt
    );

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
    UsdMayaJobImportArgs mArgs;
    std::string mFileName;
    std::map<std::string,std::string> mVariants;
    UsdMayaPrimReaderContext::ObjectRegistry mNewNodeRegistry;
    MDagPath mMayaRootDagPath;

private:

    // Data
    std::string mPrimPath;
    MDagModifier mDagModifierUndo;
    bool mDagModifierSeeded;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
