//
// Copyright 2024 Autodesk
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
#ifndef MAYAUSDAPI_CONTEXTOPS_H
#define MAYAUSDAPI_CONTEXTOPS_H

#include <mayaUsdAPI/api.h>

#include <ufe/contextOps.h>

#include <string>

namespace MAYAUSDAPI_NS_DEF {

//! Returns true if this contextOps is in Bulk Edit mode.
//! Meaning there are multiple items selected and the operation will (potentially)
//! be ran on all of the them.
//!
//! \note If the input context ops is not a UsdContextOps, returns false.
MAYAUSD_API_PUBLIC
bool isBulkEdit(const Ufe::ContextOps::Ptr&);

//! When in bulk edit mode returns the type of all the prims
//! if they are all of the same type. If mixed selection
//! then empty string is returned.
//!
//! \note If the input context ops is not a UsdContextOps, returns empty string.
MAYAUSD_API_PUBLIC
const std::string bulkEditType(const Ufe::ContextOps::Ptr&);

//! Adds the special Bulk Edit header as the first item.
//!
//! \note If the input context ops is not a UsdContextOps, this does nothing.
MAYAUSD_API_PUBLIC
void addBulkEditHeader(const Ufe::ContextOps::Ptr&, Ufe::ContextOps::Items&);

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_CONTEXTOPS_H
