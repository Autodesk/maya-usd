//
// Copyright 2019 Pixar
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
#include <mayaUsd/fileio/apiWriterRegistry.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorRfMLight.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_DEFINE_WRITER(PxrAovLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrCylinderLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDiskLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDistantLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDomeLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrEnvDayLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrRectLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrSphereLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

#if PXR_VERSION < 2209
// In old versions of USD, we'll fallback to the "old" behavior of translating a
// PxrMeshLight.
PXRUSDMAYA_DEFINE_WRITER(PxrMeshLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}
#else
PXRUSDMAYA_DEFINE_API_WRITER(PxrMeshLight, context)
{
    return UsdMayaTranslatorRfMLight::CheckAndWriteMeshLightAPI(context);
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
