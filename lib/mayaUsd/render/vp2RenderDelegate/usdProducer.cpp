//
// Copyright 2026 Autodesk
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
#include "usdProducer.h"

#include "usdImagingDelegateProducer.h"

PXR_NAMESPACE_OPEN_SCOPE

std::unique_ptr<HdVP2UsdProducer> HdVP2UsdProducer::Create()
{
    return std::make_unique<HdVP2UsdImagingDelegateProducer>();
}

PXR_NAMESPACE_CLOSE_SCOPE
