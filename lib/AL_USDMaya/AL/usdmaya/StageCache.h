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
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageCache.h"

#include "AL/maya/event/MayaEventManager.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

/// \brief  Maintains a cache of all active stages within maya
/// \ingroup usdmaya
class StageCache
{
public:

  /// \brief  Return the singleton stage cache for use by all USD clients within Maya.
  ///         2 stage caches are maintained; 1 for stages that have been
  ///         force-populated, and 1 for stages that have not been force-populated.
  /// \param  forcePopulate determine which cache to return
  /// \return the cache requested
  /// \todo we need to offer a way of mapping from a specific ProxyShape to a specific stage
  static UsdStageCache& Get(bool forcePopulate = true);

  /// \brief  Clear the cache
  static void Clear();

  /// \brief  deletes the callbacks constructed to manage the stage cache
  static void removeCallbacks();
private:
  static AL::event::CallbackId g_beforeNewCallbackId;
  static AL::event::CallbackId g_beforeLoadCallbackId;
};

} // usdmaya
} // AL
