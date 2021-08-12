//
// Copyright 2021 Autodesk
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
#ifndef HD_VP2_SELECTABILITY_H
#define HD_VP2_SELECTABILITY_H

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Determine the selectability status of a prim.
 */

class Selectability
{
public:
    /*! \brief  The possible states of selectability.
     */
    enum State
    {
        kInherit,
        kOn,
        kOff
    };

    /*! \brief  The tokens used in the selectability metadata.
     */
    static TfToken MetadataToken;
    static TfToken InheritToken;
    static TfToken OnToken;
    static TfToken OffToken;

    /*! \brief  Prepare any internal data needed for selection prior to selection queries.
     */
    static void prepareForSelection();

    /*! \brief  Compute the selectability of a prim, considering inheritence.
     */
    static bool isSelectable(UsdPrim prim);

    /*! \brief  Retrieve the local selectability state of a prim, without any inheritence.
     */
    static State getLocalState(const UsdPrim& prim);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_SELECTABILITY_H
