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

#include "apiWriterRegistry.h"

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

static std::map<std::string, UsdMayaApiWriterRegistry::WriterFn> _reg;

void UsdMayaApiWriterRegistry::Register(const std::string& writerId, const WriterFn& fn)
{
    const bool inserted = _reg.insert({ writerId, fn }).second;
    if (!inserted) {
        TF_CODING_ERROR("Duplicate registration for %s.", writerId.c_str());
    }
}

const std::map<std::string, UsdMayaApiWriterRegistry::WriterFn>& UsdMayaApiWriterRegistry::GetAll()
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaApiWriterRegistry>();
    return _reg;
}

PXR_NAMESPACE_CLOSE_SCOPE
