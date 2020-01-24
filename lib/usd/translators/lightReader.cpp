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

#include "../../fileio/translators/translatorRfMLight.h"
#include "../../fileio/primReaderRegistry.h"

#include "pxr/usd/usdLux/cylinderLight.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdRi/pxrAovLight.h"
#include "pxr/usd/usdRi/pxrEnvDayLight.h"

PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_DEFINE_READER(UsdLuxCylinderLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDiskLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDistantLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxDomeLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxGeometryLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxRectLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdLuxSphereLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrAovLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}

PXRUSDMAYA_DEFINE_READER(UsdRiPxrEnvDayLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Read(args, context);
}


PXR_NAMESPACE_CLOSE_SCOPE
