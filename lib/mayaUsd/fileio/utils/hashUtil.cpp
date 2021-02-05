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
#include "hashUtil.h"

#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>

PXR_NAMESPACE_OPEN_SCOPE

bool UsdMayaHashUtil::GenerateMD5DigestFromByteStream(
    const uint8_t* data,
    const size_t   len,
    char           digest[32])
{
    // TODO: (yliangsiew) Implement maybe by copying RFC 1321 C implementation directly instead of
    // using Boost.
    boost::uuids::detail::md5              md5Hash;
    boost::uuids::detail::md5::digest_type md5Digest;

    md5Hash.process_bytes(data, len);
    md5Hash.get_digest(md5Digest);

    const char* digestStr = reinterpret_cast<const char*>(&md5Digest);
    boost::algorithm::hex(
        digestStr, digestStr + sizeof(boost::uuids::detail::md5::digest_type), digest);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
