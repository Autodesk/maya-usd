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

#ifndef AL_USDMAYA_TRANSLATORPROXYSHAPE_H
#define AL_USDMAYA_TRANSLATORPROXYSHAPE_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"

PXR_NAMESPACE_OPEN_SCOPE

/// This translator works with pixar's usdExport command as opposed to the
/// translators contained in fileio.
struct AL_USDMayaTranslatorProxyShape {
  /// This method generates a USD prim with a model reference
  /// when provided args and a context that identify an
  /// AL_usdmaya_ProxyShape node.
  PXRUSDMAYA_API
  static bool Create(
          const PxrUsdMayaPrimWriterArgs &args,
          PxrUsdMayaPrimWriterContext *context);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //AL_USDMAYA_TRANSLATORPROXYSHAPE_H
