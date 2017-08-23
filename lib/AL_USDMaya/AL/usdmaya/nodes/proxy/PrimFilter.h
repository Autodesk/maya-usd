//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include <vector>
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

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
  PrimFilter(const SdfPathVector& previousPrims, const std::vector<UsdPrim>& newPrimSet, nodes::ProxyShape* proxy);

  /// \brief  returns the set of prims to create
  inline const std::vector<UsdPrim>& newPrimSet() const
    { return m_newPrimSet; }

  /// \brief  returns the set of prims that require created transforms
  inline const std::vector<UsdPrim>& transformsToCreate() const
    { return m_transformsToCreate; }

  /// \brief  returns the list of prims that needs to be updated
  inline const std::vector<UsdPrim>& updatablePrimSet() const
    { return m_updatablePrimSet; }

  /// \brief  returns the list of prims that have been removed from the stage
  inline const SdfPathVector& removedPrimSet() const
    { return m_removedPrimSet; }

private:
  std::vector<UsdPrim> m_newPrimSet;
  std::vector<UsdPrim> m_transformsToCreate;
  std::vector<UsdPrim> m_updatablePrimSet;
  SdfPathVector m_removedPrimSet;
};

//----------------------------------------------------------------------------------------------------------------------
} // proxy
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

