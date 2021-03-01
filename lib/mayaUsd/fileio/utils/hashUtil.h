//
// Copyright 2020 Apple
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
#ifndef PXRUSDMAYA_HASHUTIL_H
#define PXRUSDMAYA_HASHUTIL_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdMayaHashUtil {
MAYAUSD_CORE_PUBLIC
bool GenerateMD5DigestFromByteStream(
    const uint8_t* data,
    const size_t   len,
    unsigned char  digest[16]);
}; // namespace UsdMayaHashUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXRUSDMAYA_HASHUTIL_H */
