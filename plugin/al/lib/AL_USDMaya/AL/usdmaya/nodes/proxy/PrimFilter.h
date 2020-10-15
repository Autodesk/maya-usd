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

#include <AL/usdmaya/Api.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <mayaUsdUtils/ForwardDeclares.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The prim filter needs to know about some state provided in the proxy shape node. In
/// order to maintain a
///         separation between the filter and the proxy (so that it's easy to test!), this class
///         acts as a bridge between the two.
//----------------------------------------------------------------------------------------------------------------------
struct PrimFilterInterface
{
    /// \brief  Given a path to a prim, this method will return some translator type information for
    /// the prim found at that path,
    ///         which the proxy shape has previously cached (i.e. the old state of the prim prior to
    ///         a variant switch). If the proxy shape is aware of the prim, and the returned info is
    ///         valid, true will be returned. If the proxy shape is unaware of the prim (i.e. a
    ///         variant switch has created it), then false will be returned.
    /// \param  path the path to the prim we are querying
    /// \return the translatorId string, or an empty string if unknown type
    virtual std::string getTranslatorIdForPath(const SdfPath& path) = 0;

    /// \brief  for a specific translator, this method should return whether it supports update, and
    /// if that translator requires a
    ///         DAG path to be created.
    /// \param  translatorId the translator to query
    /// \param  supportsUpdate returned value that indicates if the translator in question can be
    /// updated \param  requiresParent returned value that indicates whether the translator in
    /// question needs a DAG path to be created \param  importableByDefault returned value that
    /// indicates whether the prim needs to be forced into being \return returns false if the
    /// translator is unknown, true otherwise
    virtual bool getTranslatorInfo(
        const std::string& translatorId,
        bool&              supportsUpdate,
        bool&              requiresParent,
        bool&              importableByDefault)
        = 0;

    virtual std::string generateTranslatorId(UsdPrim prim) = 0;

    /// \brief  check if a prim is dirty.
    /// \param  prim the prim to check.
    /// \return returns true if yes, false otherwise.
    virtual bool isPrimDirty(const UsdPrim& prim) = 0;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class to filter the prims during a variant switch
//----------------------------------------------------------------------------------------------------------------------
class PrimFilter
{
public:
    /// \brief  constructor constructs the prim filter
    /// \param  previousPrims the previous set of prims that existed in the stage
    /// \param  newPrimSet the new set of prims that have been created
    /// \param  proxy the proxy shape
    /// \param  forceImport mirrors the status of the -fi flag
    AL_USDMAYA_PUBLIC
    PrimFilter(
        const SdfPathVector&               previousPrims,
        const MayaUsdUtils::UsdPrimVector& newPrimSet,
        PrimFilterInterface*               proxy,
        bool                               forceImport = false);

    /// \brief  returns the set of prims to create
    inline const std::vector<UsdPrim>& newPrimSet() const { return m_newPrimSet; }

    /// \brief  returns the set of prims that require created transforms
    inline const std::vector<UsdPrim>& transformsToCreate() const { return m_transformsToCreate; }

    /// \brief  returns the list of prims that needs to be updated
    inline const std::vector<UsdPrim>& updatablePrimSet() const { return m_updatablePrimSet; }

    /// \brief  returns the list of prims that have been removed from the stage
    inline const SdfPathVector& removedPrimSet() const { return m_removedPrimSet; }

private:
    std::vector<UsdPrim> m_newPrimSet;
    std::vector<UsdPrim> m_transformsToCreate;
    std::vector<UsdPrim> m_updatablePrimSet;
    SdfPathVector        m_removedPrimSet;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace proxy
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
