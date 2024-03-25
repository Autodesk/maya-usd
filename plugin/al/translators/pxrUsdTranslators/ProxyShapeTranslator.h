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

#ifndef AL_USDMAYA_PROXYSHAPETRANSLATOR_H
#define AL_USDMAYA_PROXYSHAPETRANSLATOR_H

#include "usdMaya/api.h"

#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>

#include <pxr/pxr.h>

#if PXR_VERSION >= 2403
#include <optional>
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// This translator works with pixar's usdExport command as opposed to the
/// translators contained in fileio.
struct AL_USDMayaTranslatorProxyShape
{
    /// This method generates a USD prim with a model reference
    /// when provided args and a context that identify an
    /// AL_usdmaya_ProxyShape node.
    PXRUSDMAYA_API
    static bool Create(const UsdMayaPrimWriterArgs& args, UsdMayaPrimWriterContext* context);

private:
    /// Return true if \p field should be copied from the spec at \p srcPath in
    /// \p srcLayer to the spec at \p dstPath in \p dstLayer.
    /// This version overrides the default behavior to preserve values that
    /// already exist on dest if source does not have them (otherwise they
    /// would be cleared).
    static bool _ShouldGraftValue(
        SdfSpecType           specType,
        const TfToken&        field,
        const SdfLayerHandle& srcLayer,
        const SdfPath&        srcPath,
        bool                  fieldInSrc,
        const SdfLayerHandle& dstLayer,
        const SdfPath&        dstPath,
        bool                  fieldInDst,
#if PXR_VERSION >= 2403
        std::optional<VtValue>* valueToCopy)
#else
        boost::optional<VtValue>* valueToCopy)
#endif
    {
        // SdfShouldCopyValueFn copies everything by default
        return (!fieldInDst && fieldInSrc);
    }

    static bool _ShouldGraftChildren(
        const TfToken&        childrenField,
        const SdfLayerHandle& srcLayer,
        const SdfPath&        srcPath,
        bool                  fieldInSrc,
        const SdfLayerHandle& dstLayer,
        const SdfPath&        dstPath,
        bool                  fieldInDst,
#if PXR_VERSION >= 2403
        std::optional<VtValue>* srcChildren,
        std::optional<VtValue>* dstChildren)
#else
        boost::optional<VtValue>* srcChildren,
        boost::optional<VtValue>* dstChildren)
#endif
    {
        // SdfShouldCopyChildrenFn copies everything by default
        return true;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AL_USDMAYA_PROXYSHAPETRANSLATOR_H
