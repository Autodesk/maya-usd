//
// Copyright 2022 Pixar
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
#ifndef PXRUSDMAYA_API_WRITER_REGISTRY_H
#define PXRUSDMAYA_API_WRITER_REGISTRY_H

#include <mayaUsd/fileio/primWriterRegistry.h>

#include <pxr/pxr.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaApiWriterContext;

/// API writers are plugins that can apply API schemas to exported UsdPrims.
///
/// For each prim that is exported, we will then run all of the API Writers (in
/// writerId order).
class UsdMayaApiWriterRegistry
{
public:
    using WriterFn = std::function<bool(UsdMayaApiWriterContext*)>;

    /// Registers \p fn as an API writer.  This will be identified by \p
    /// writerId.
    static void Register(const std::string& writerId, const WriterFn& fn);

    /// Returns all a map of all the API writers.
    static const std::map<std::string, WriterFn>& GetAll();

private:
};

/// This macro is for handling the case of a maya type that corresponds to an
/// API schema.  In particular, the mayaTypeName should *not* result in a new
/// UsdPrim.
#define PXRUSDMAYA_DEFINE_API_WRITER(mayaTypeName, ctxVarName)                               \
    struct UsdMayaApiWriter_##mayaTypeName                                                   \
    {                                                                                        \
    };                                                                                       \
    static bool UsdMaya_ApiWriter_##mayaTypeName(UsdMayaApiWriterContext*);                  \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimWriterRegistry, UsdMayaWriter_##mayaTypeName)   \
    {                                                                                        \
        UsdMayaPrimWriterRegistry::RegisterPrimless(#mayaTypeName);                          \
    }                                                                                        \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaApiWriterRegistry, UsdMayaWriter_##mayaTypeName)    \
    {                                                                                        \
        UsdMayaApiWriterRegistry::Register(#mayaTypeName, UsdMaya_ApiWriter_##mayaTypeName); \
    }                                                                                        \
    bool UsdMaya_ApiWriter_##mayaTypeName(UsdMayaApiWriterContext* ctxVarName)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
