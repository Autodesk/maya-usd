#ifndef PXRUSDMAYA_HASHUTIL_H
#define PXRUSDMAYA_HASHUTIL_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdMayaHashUtil {
MAYAUSD_CORE_PUBLIC
bool GenerateMD5DigestFromByteStream(const uint8_t* data, const size_t len, char digest[32]);
}; // namespace UsdMayaHashUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXRUSDMAYA_HASHUTIL_H */
